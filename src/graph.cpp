#include "graph.h"
#include <deque>
#include <gtest/gtest.h>

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

TEST(Graph, SingleNode) {
    int R = 20;
    NGraph::World wld(R);

    auto bot = wld.get_root();

    wld.SMove(bot, unitZ);

    NGraph::BotNode::Ptr b1, b2;
    std::tie(b1, b2) = wld.Fission(bot, unitY); // bot is None.

    wld.SMove(b1, unitZ);
    wld.Fill(b2, unitY);

    Region r(Vec3(0, 2, 0), Vec3(0, 0, 0));
    wld.GVoid({b1, b2}, r);

    wld.SMove(b2, unitY);
    wld.SMove(b1, unitZ);

    b1 = wld.Fusion(b1, b2); // b1, b2 is None.

    auto rq = wld.nanobots_required();
    for (auto i = rq.begin(); i!= rq.end(); ++i) {
        LOG() << i->first << " : " << i->second << "\n";
    }

    wld.create_trace();
    for (auto pn : wld.all_nodes()) {
        LOG() << &pn << " : ";
        for (auto i : pn->bids) {
            std::cerr << i << ", ";
        }
        std::cerr << "\n";
    }

}

namespace NGraph {
struct TimeFrame {
    std::vector<Action::Ptr> actions;
};

bool bfs(BotNode::Ptr root, bool up_to_down, std::function<bool(BotNode&)> visitor) {
    std::set<BotNode*> visited;
    std::deque<BotNode*> queue;
    queue.push_back(root.get());

    while (!queue.empty()) {
        auto p = queue.front(); queue.pop_front();
        if (visited.find(p) != visited.end()) continue;
        visitor(*p);
        visited.insert(p);

        if (up_to_down) {
            for (auto c : p->children) {
                queue.push_back(c);
            }
        } else {
            for (auto c : p->parents) {
                queue.push_back(c.get());
            }
        }
    }

    return true;
}

std::vector<BotNode*> World::all_nodes() {
     std::vector<BotNode*> res;
     bfs(get_root(), true, [&](BotNode& b) {
         res.push_back(&b);
         return true;
     });
     return res;
}

std::map<BotNode*, int> World::nanobots_required() {
    // find max splits in downstream nodes.

    std::map<BotNode*, int> dp;
    bfs(get_leaf(), false, [&](BotNode& node) {
        dp[&node] = 0;
        return true;
    });
    dp[get_leaf().get()] = 1;

    bfs(get_leaf(), false, [&](BotNode& node) {
        for (auto p : node.parents) {
            dp[p.get()] += dp[&node];
        }
        return true;
    });

    return dp;
}

template <typename T>
void sort_bots(T& bots) {
    std::sort(bots.begin(), bots.end(), [](const BotNode::Ptr& lhs, const BotNode::Ptr& rhs) {
        ASSERT(!lhs->bids.empty());
        ASSERT(!rhs->bids.empty());
        return lhs->bids.front() < rhs->bids.front();
    });
}

Trace World::create_trace() {

    // assign nodes.
    auto node2demand = nanobots_required();
    for (int i = 0; i < node2demand[get_root().get()]; ++i) {
        get_root()->bids.push_back(i);
        get_root()->bids_work.push_back(i);
    }
    LOG() << get_root()->bids.size() << "\n";

    bfs(get_root(), true, [&](BotNode& node) {
        if (node.parents.size() == 1) {
            auto& parent = *node.parents[0];
            for (auto it = parent.bids_work.begin(); it != parent.bids_work.end();) {
                node.bids.push_back(*it);
                node.bids_work.push_back(*it);
                it = parent.bids_work.erase(it);
                if (node.bids_work.size() == node2demand[&node]) {
                    break;
                }
            }
        } else if (node.parents.size() == 2) {
            node.bids = node.parents[0]->bids_work;
            std::copy(node.parents[1]->bids_work.begin(),node.parents[1]->bids_work.end(),  
                std::back_inserter(node.bids));
            node.bids_work = node.bids;
        } else if (node.parents.size() == 0) {
            ASSERT(&node == get_root().get());
        }
        return true;
    });

    LOG() << "ASSIGN NODES DONE\n";

    std::vector<BotNode::Ptr> current_nodes { get_root() };


    // pending action is waiting the bots.
    // if nodes are empty, execute action.
    struct Pending {
        std::vector<BotNode*> arrived;
        std::vector<BotNode*> unarrived;
    };
    std::map<Action*, Pending> pending_bots; 
    auto is_pending = [&](const BotNode::Ptr& node) {
        for (auto it = pending_bots.begin(); it != pending_bots.end(); ++it) {
            auto& arrived = it->second.arrived;
            auto it2 = std::find(arrived.begin(), arrived.end(), node.get());
            if (it2 != arrived.end()) {
                return true;
            }
        }
        return false;
    };
    auto add_pending = [&](const BotNode::Ptr& node, const Action::Ptr& action) {
        auto it = pending_bots.find(action.get());
        if (it == pending_bots.end()) {
            // add new. all unarrived except this one.
            Pending pend;
            for (auto n : action->member) {
                if (n == node) {
                    pend.arrived.push_back(n.get());
                } else {
                    pend.unarrived.push_back(n.get());
                }
            }
            pending_bots.insert(std::make_pair(action.get(), pend));
        } else {
            // add existing. not arrived -> arrived.
            it->second.arrived.push_back(node.get());
            it->second.unarrived.erase(
                std::find(it->second.unarrived.begin(), it->second.unarrived.end(), node.get()));
        }
        return false;
    };



    typedef std::vector<std::pair<BotID, Command>> Stage;
    Trace trace;
    struct EmitGroup {
        Stage& stage;
        std::vector<BotNode::Ptr> member;
        BotNode::Ptr node;
        bool bots_changed = false;
        EmitGroup(Stage& stage_, const std::vector<BotNode::Ptr>& member_)
            : stage(stage_)
            , member(member_) {
            sort_bots(member);
            node = member[0];
        }
        void operator()(Action& action) {
            if (auto tmp = dynamic_cast<ActionWait*>(&action)) { operator()(*tmp); }
            if (auto tmp = dynamic_cast<ActionSMove*>(&action)) { operator()(*tmp); }
            if (auto tmp = dynamic_cast<ActionFission*>(&action)) { operator()(*tmp); }
            if (auto tmp = dynamic_cast<ActionFill*>(&action)) { operator()(*tmp); }

            if (auto tmp = dynamic_cast<ActionFusion*>(&action)) { operator()(*tmp); }
            if (auto tmp = dynamic_cast<ActionGVoid*>(&action)) { operator()(*tmp); }
        }
        void operator()(ActionWait& action) {
            stage.emplace_back(node->bid(), CommandWait{});
        }
        void operator()(ActionSMove& action) {
            stage.emplace_back(node->bid(), CommandSMove{action.lld});
        }
        void operator()(ActionFission& action) {
            auto nd = action.n2->pos - node->pos;
            stage.emplace_back(node->bid(), CommandFission{nd});
            bots_changed = true;
        }
        void operator()(ActionFill& action) {
            stage.emplace_back(node->bid(), CommandFill{action.nd});
        }

        void operator()(ActionFusion action) {
            ASSERT(member.size() == 2);
            auto nd = member[1]->pos - member[0]->pos;
            if (member[0] == action.remain) {
                stage.emplace_back(member[0]->bid(), CommandFusionP{nd});
                stage.emplace_back(member[1]->bid(), CommandFusionS{-nd});
            } else {
                stage.emplace_back(member[0]->bid(), CommandFusionS{nd});
                stage.emplace_back(member[1]->bid(), CommandFusionP{-nd});
            }
            bots_changed = true;
        }
        void operator()(ActionGVoid action) {
            ASSERT(member.size() == 1 || member.size() == 2 || member.size() == 4);
            // TODO.
        }
    };

    do {
        bool current_nodes_changed = false;

        // accept new arrivals.
        for (size_t i = 0; i < current_nodes.size(); ++i) { // bid order.
            if (is_pending(current_nodes[i])) {
                // do not take the next action.
                auto action = std::make_shared<ActionWait>();
                action->member.push_back(current_nodes[i]);
                add_pending(current_nodes[i], action);
            } else {
                // take action and add too pool.
                add_pending(current_nodes[i], current_nodes[i]->next());
            }
        }
        LOG() << "ACCEPT arrivals DONE\n";

        // stage all actions which are ready.
        Stage stage;
        for (auto it = pending_bots.begin(); it != pending_bots.end();) {
            if (it->second.unarrived.empty()) { // no one is blocked -> released.
                Action& action = *it->first;
                EmitGroup emit {stage, action.member};
                //emit(action);

                if (emit.bots_changed) {
                    current_nodes_changed = true;
                }

                it = pending_bots.erase(it);
            } else {
                ++it;
            }
        }
        ASSERT(stage.size() == current_nodes.size());
        LOG() << "release DONE\n";

        // to trace in BID order.
        std::sort(stage.begin(), stage.end(), [](auto lhs, auto rhs) {
            return lhs.first < rhs.first;
        });

        for (auto item : stage) {
            trace.push_back(item.second);
        }

        if (current_nodes_changed) {
            sort_bots(current_nodes);
        }
    } while (!(current_nodes.size() == 1 && current_nodes[0] == get_leaf()));
}

}
