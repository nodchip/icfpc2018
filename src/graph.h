#pragma once

#include <memory>
#include <boost/optional.hpp>

#include "command.h"
#include "trace.h"
#include "system.h"
#include "state.h"
#include "vec3.h"
#include "nanobot.h"
#include "region.h"
#include "matrix.h"
#include "log.h"

#define DEF_PTRS(cls) \
    typedef std::shared_ptr<cls> Ptr; 

namespace NGraph {

typedef uint32_t Time;

struct BotNode;

struct Action {
    DEF_PTRS(Action);
    virtual ~Action() {}

    std::vector<std::shared_ptr<BotNode>> member;
};

struct ActionWait : public Action { };
struct ActionSMove : public Action { Vec3 lld; };
struct ActionFission : public Action { std::shared_ptr<BotNode> n1, n2; };
struct ActionFusion : public Action { std::shared_ptr<BotNode> remain; };
struct ActionFill : public Action { Vec3 nd; };
struct ActionGVoid : public Action {
     Region region = {Vec3(0, 0, 0), Vec3(0,0,0)};
};


struct World;

struct BotNode : std::enable_shared_from_this<BotNode> {
    DEF_PTRS(BotNode);

    explicit BotNode() {
    }
    explicit BotNode(const BotNode::Ptr& prev) {
        parents.push_back(prev);
        prev->children.push_back(this);
    }
    explicit BotNode(const BotNode::Ptr& prev1, const BotNode::Ptr& prev2) {
        parents.push_back(prev1);
        parents.push_back(prev2);
        prev1->children.push_back(this);
        prev2->children.push_back(this);
    }

    std::vector<BotNode::Ptr> parents;
    std::vector<BotNode*> children;
    Vec3 pos;
    std::vector<Action::Ptr> history;

    Action::Ptr next() {
        return history[cursor++];
    }

    BotID bid() const {
        ASSERT(!bids.empty());
        return bids[0];
    }

    // used in create_trace.
    std::vector<BotID> bids; // bid + seeds
    std::vector<BotID> bids_work; 
    size_t cursor = 0;
};


struct World {
    DEF_PTRS(World);

    Matrix matrix;

    BotNode::Ptr root;
    std::vector<BotNode::Ptr> current_bots;

    Time t = 0;

    World(int R) : matrix(R) {
        root = std::make_shared<BotNode>();
        current_bots.push_back(root);
    }

    BotNode::Ptr get_root() {
        return root;
    }

    BotNode::Ptr get_leaf() {
        ASSERT(current_bots.size() == 1);
        return current_bots[0];
    }
    std::vector<BotNode*> all_nodes();

    void add_active(BotNode::Ptr& bot) {
        current_bots.push_back(bot);
    }
    void remove_active(BotNode::Ptr& bot) {
        current_bots.erase(
            std::find(current_bots.begin(), current_bots.end(), bot));
        bot.reset();
    }

    std::tuple<BotNode::Ptr, BotNode::Ptr> Fission(BotNode::Ptr& bot, Vec3 lld) {
        BotNode::Ptr b1 = std::make_shared<BotNode>(bot);
        b1->pos = bot->pos;
        BotNode::Ptr b2 = std::make_shared<BotNode>(bot);
        b2->pos = bot->pos + lld;

        LOG() << "bot fission: " << bot->pos << "->" << b1->pos << ", " << b2->pos << "\n";

        auto action = std::make_shared<ActionFission>();
        action->member.push_back(bot);
        action->n1 = b1;
        action->n2 = b2;
        bot->history.push_back(action);

        add_active(b1);
        add_active(b2);
        remove_active(bot);
        return {b1, b2};
    }

    BotNode::Ptr Fusion(BotNode::Ptr& bremain, BotNode::Ptr& berase) {
        BotNode::Ptr bot = std::make_shared<BotNode>(bremain, berase);
        bot->pos = bremain->pos;

        LOG() << "bot fusion: " << bremain->pos << " & " << berase->pos << "->" << bot->pos << "\n";

        auto action = std::make_shared<ActionFusion>();
        action->member.push_back(bremain);
        action->member.push_back(berase);
        action->remain = bremain;
        bremain->history.push_back(action);
        berase->history.push_back(action);

        remove_active(bremain);
        remove_active(berase);
        add_active(bot);
        return bot;
    }

    bool GVoid(const std::vector<BotNode::Ptr>& bots, Region r) {
        auto action = std::make_shared<ActionGVoid>();
        for (BotNode::Ptr b : bots) {
            action->member.push_back(b);
            b->history.push_back(action);
        }
        action->region = r.canonical();

        for (int z = r.c1.z; z <= r.c2.z; ++z) {
            for (int y = r.c1.y; y <= r.c2.y; ++y) {
                for (int x = r.c1.x; x <= r.c2.x; ++x) {
                    matrix(x, y, z) = Voxel::Void;
                }
            }
        }
        LOG() << "bot group void: ";
        for (auto b : bots) {
            std::cerr << b->pos << " ";
        };
        std::cerr << " voids " << action->region.c1 << " ~ " << action->region.c2 << "\n";

        return true;
    }
    
    bool SMove(BotNode::Ptr& bot, Vec3 lld) {
        auto action = std::make_shared<ActionSMove>();
        action->lld = lld;
        bot->history.push_back(action);
        LOG() << "bot move: " << bot->pos << "->" << (bot->pos + lld) << "\n";

        bot->pos += lld;
        return true;
    }

    bool Fill(BotNode::Ptr& bot, Vec3 nd) {
        auto action = std::make_shared<ActionFill>();
        action->nd = nd;
        bot->history.push_back(action);
        matrix(bot->pos + nd) = Voxel::Full;
        LOG() << "bot fill: " << bot->pos << " fills " << (bot->pos + nd) << "\n";
        return true;
    }

    Trace create_trace();

    std::map<BotNode*, int> nanobots_required();
};


#undef DEF_PTRS

}
