#include "nmms.h"
// std
#include <map>
#include <unordered_map>
// 3rd
#ifdef TEST_PROJECT
#include <gtest/gtest.h>
#endif // TEST_PROJECT
#include "debug_message.h"

namespace NOutputTrace {
    struct LDEncoding {
        static LDEncoding from_SLD(Vec3 ld) {
            // TODO: assert ld is a SLD.
            if (ld.x != 0) {
                return LDEncoding(0b01, ld.x + 5);
            } else if (ld.y != 0) {
                return LDEncoding(0b10, ld.y + 5);
            } else if (ld.z != 0) {
                return LDEncoding(0b11, ld.z + 5);
            }
            //ASSERT(false);
            return LDEncoding(0xff, 0xff);
        }
        static LDEncoding from_LLD(Vec3 ld) {
            // TODO: assert ld is a LLD.
            if (ld.x != 0) {
                return LDEncoding(0b01, ld.x + 15);
            } else if (ld.y != 0) {
                return LDEncoding(0b10, ld.y + 15);
            } else if (ld.z != 0) {
                return LDEncoding(0b11, ld.z + 15);
            }
            //ASSERT(false);
            return LDEncoding(0xff, 0xff);
        }
        uint8_t a = 0; // axis
        uint8_t i = 0; // length

    private:
        LDEncoding(uint8_t a_, uint8_t i_) : a(a_), i(i_) {}
    };
}  // namespace NOutputTrace

namespace NProceedTimestep {

    struct GroundConnectivityChecker {
        std::vector<Vec3> filled_voxels;

        void fill(Vec3 p) {
            filled_voxels.push_back(p);
        }

        bool new_voxels_are_grounded(/* no const for union find */System& system) {
            const auto n6 = neighbors6();
            const uint32_t ground_group = Vec3(0, 0, 0).index();
            for (auto& p : filled_voxels) {
                bool grounded = false;
                for (auto offset : n6) {
                    auto c = p + offset;
                    if (system.ground_and_full_voxels.findSet(c.index(), ground_group)) {
                        grounded = true;
                        break;
                    }
                }
                if (!grounded) {
                    return false;
                }
            }
            return true;
        }

        bool update(System& system) {
            const auto n6 = neighbors6();
            for (auto& p : filled_voxels) {
                for (auto offset : n6) {
                    auto c = p + offset;
                    if (system.matrix(c)) {
                        system.ground_and_full_voxels.unionSet(p.index(), c.index());
                    }
                }
            }
            return true;
        }
    };

    constexpr int k_BotIDs = 21;
    struct FusionStage {
        FusionStage() {
            std::memset(fusion, 0, sizeof(int) * k_BotIDs * k_BotIDs);
        }

        void addPS(BotID me, BotID other) {
            fusion[me /* remaining */][other /* erased */] += 1;
        }
        void addSP(BotID me, BotID other) {
            fusion[other][me] += 1;
        }

        int fusion[k_BotIDs][k_BotIDs];

        bool update(System& sys) {
            for (int i = 0; i < k_BotIDs; ++i) {
                for (int j = 0; j < k_BotIDs; ++j) {
                    if (fusion[i][j]) {
                        if (fusion[i][j] != 2) return false;

                        const int idx_remain = sys.bot_index_by(i);
                        const int idx_erase = sys.bot_index_by(j);
                        if (idx_remain < 0 || idx_erase < 0) return false;

                        std::copy(sys.bots[idx_erase].seeds.begin(),
                             sys.bots[idx_erase].seeds.end(),
                             std::back_inserter(sys.bots[idx_remain].seeds));
                        sys.bots[idx_remain].seeds.push_back(
                            sys.bots[idx_erase].bid);
                        sys.bots.erase(sys.bots.begin() + idx_erase);

                        sys.energy += Costs::k_Fusion;
                    }
                }
            }
            return true;
        }
    };

    struct UpdateSystem : public boost::static_visitor<bool> {
        System& sys;
        Bot& bot;
        bool& halt_requested;

        FusionStage& fusion_stage;
        GroundConnectivityChecker& ground_connectivity_checker;

        UpdateSystem(System& sys_, Bot& bot_, bool& halt_requested_,
                FusionStage& fusion_stage_, GroundConnectivityChecker& ground_connectivity_checker_)
             : sys(sys_)
             , bot(bot_)
             , halt_requested(halt_requested_)
             , fusion_stage(fusion_stage_)
             , ground_connectivity_checker(ground_connectivity_checker_) {
             }
        bool operator()(CommandHalt) {
            if (sys.bots.size() != 1 || sys.bots[0].pos != sys.final_pos()) {
                LOG_ERROR("[CommandHalt] preconditions are not met.");
                return false;
            }
            halt_requested = true;
            return true;
        }
        bool operator()(CommandWait) {
             return true;
        }
        bool operator()(CommandFlip) {
            // XXX: whether flipping twice in the same timestep is allowed is not clearly stated.
            sys.harmonics_high = !sys.harmonics_high;
            return true;
        }
        bool operator()(CommandSMove cmd) {
            if (!sys.matrix.is_in_matrix(bot.pos + cmd.lld)) {
                LOG_ERROR("[CommandSMove] target position out of range.");
                return false;
            }
            if (sys.matrix.any_full(Region(bot.pos, bot.pos + cmd.lld))) {
                LOG_ERROR("[CommandSMove] some full voxels in between the move.");
                return false;
            }
            bot.pos += cmd.lld;
            sys.energy += Costs::k_SMove * mlen(cmd.lld);
            return true;
        };
        bool operator()(CommandLMove cmd) {
            auto c1 = bot.pos + cmd.sld1;
            auto c2 = c1 + cmd.sld2;
            if (!sys.matrix.is_in_matrix(c1)) {
                LOG_ERROR("[CommandLMove] c1 out of range");
                return false;
            }
            if (!sys.matrix.is_in_matrix(c2)) {
                LOG_ERROR("[CommandLMove] c2 out of range");
                return false;
            }
            if (sys.matrix.any_full(Region(bot.pos, c1))) {
                LOG_ERROR("[CommandLMove] some full voxels in between the move.");
                return false;
            }
            if (sys.matrix.any_full(Region(c1, c2))) {
                LOG_ERROR("[CommandLMove] some full voxels in between the move.");
                return false;
            }
            bot.pos = c2;
            sys.energy += Costs::k_LMove * (mlen(cmd.sld1) + Costs::k_LMoveOffset + mlen(cmd.sld2));
            return true;
        };
        bool operator()(CommandFill cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) {
                LOG_ERROR("[CommandFill] target voxel out of range");
                return false;
            }
            if (sys.matrix(c) == Void) {
                sys.matrix(c) = Full;
                ground_connectivity_checker.fill(c);
                sys.energy += Costs::k_FillVoid;
            } else {
                sys.energy += Costs::k_FillFull;
            }
            return true;
        };
        bool operator()(CommandFission cmd) {
            auto c = bot.pos + cmd.nd;
            const int n_bots = int(bot.seeds.size());
            if (n_bots == 0 || n_bots <= cmd.m || cmd.m < 0) {
                LOG_ERROR("[CommandFission] preconditions are not met");
                return false;
            }
            if (!sys.matrix.is_in_matrix(c)) {
                LOG_ERROR("[CommandFission] target voxel out of range");
                return false;
            }
            // original  [bid1, bid2, .. bidm, bidm+1, .. bidn]
            // new bot    bid1
            // new seed        [bid2, .. bidm]
            // orig.seed                       [bidm+1, .. bidn]
            std::sort(bot.seeds.begin(), bot.seeds.end());
            auto it = bot.seeds.begin();
            Bot new_bot = {*it, c, {}};
            it = bot.seeds.erase(it);
            while (int(new_bot.seeds.size()) < cmd.m) {
                new_bot.seeds.push_back(*it);
                it = bot.seeds.erase(it);
            }
            sys.bots.push_back(new_bot);
            sys.energy += Costs::k_Fission;
            return true;
        };
        bool operator()(CommandFusionP cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) {
                LOG_ERROR("[CommandFusionP] target voxel out of range");
                return false;
            }
            fusion_stage.addPS(bot.bid, sys.bid_at(c));
            return true;
        };
        bool operator()(CommandFusionS cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) {
                LOG_ERROR("[CommandFusionS] target voxel out of range");
                return false;
            }
            fusion_stage.addSP(bot.bid, sys.bid_at(c));
            return true;
        };
    };
}

#ifdef TEST_PROJECT
TEST(Commands, Fission) {
    System system(4);
    int m = 2;

    bool halt = false;
    NProceedTimestep::GroundConnectivityChecker ground_connectivity_checker;
    // fission
    {
        NProceedTimestep::FusionStage fusion_stage;
        NProceedTimestep::UpdateSystem visitor(system, system.bots[0], halt, fusion_stage, ground_connectivity_checker);
        Command cmd = CommandFission{Vec3 {0, 0, 1}, m};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }

    system.print_detailed();

    ASSERT_EQ(system.bots.size(), 2);
    EXPECT_EQ(system.bots[0].seeds.size(), 18 - m);
    EXPECT_EQ(system.bots[1].seeds.size(), m);

    // fusion.
    NProceedTimestep::FusionStage fusion_stage;
    {
        NProceedTimestep::UpdateSystem visitor(system, system.bots[0], halt, fusion_stage, ground_connectivity_checker);
        Command cmd = CommandFusionS{Vec3 {0, 0, 1}};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }
    {
        NProceedTimestep::UpdateSystem visitor(system, system.bots[1], halt, fusion_stage, ground_connectivity_checker);
        Command cmd = CommandFusionP{Vec3 {0, 0, -1}};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }
    EXPECT_TRUE(fusion_stage.update(system));

    system.print_detailed();

    ASSERT_EQ(system.bots.size(), 1);
    EXPECT_EQ(system.bots[0].seeds.size(), 19);
}
#endif // TEST_PROJECT

void global_energy_update(System& system) {
    if (system.harmonics_high) {
        system.energy += Costs::k_HighHarmonics * system.matrix.R * system.matrix.R * system.matrix.R;
    } else {
        system.energy += Costs::k_LowHarmonics * system.matrix.R * system.matrix.R * system.matrix.R;
    }
    system.energy += Costs::k_Bot * system.bots.size();
}

bool proceed_timestep(System& system) {
    bool halt = false;
    NProceedTimestep::FusionStage fusion_stage;
    NProceedTimestep::GroundConnectivityChecker ground_connectivity_checker;

    // systemwide energy (count the nuber of bots before fusion/fission).
    global_energy_update(system);

    // bots consume trace in the ascending order of bids.
    system.sort_by_bid();


    const size_t n = system.bots.size();
    for (size_t i = 0; i < n; ++i) {
        Command cmd = system.trace.front(); system.trace.pop_front();
        ++system.consumed_commands; // FusionP and FusionS are treated as separate commands.

        NProceedTimestep::UpdateSystem visitor(system, system.bots[i], halt, fusion_stage, ground_connectivity_checker);
        if (!boost::apply_visitor(visitor, cmd)) {
            std::fprintf(stderr, "Error while processing trace for bot %ld\n", i);
            system.print_detailed();
            throw std::runtime_error("wrong command");
        }
    }

    fusion_stage.update(system);

    // if there are any floating voxels added in this phase while harmonics is low,
    // it is ill-formed.
    if (!system.harmonics_high && !ground_connectivity_checker.new_voxels_are_grounded(system)) {
        std::printf("Detected violation in ground connectivity in low-harmonics mode!\n");
        //throw std::runtime_error("Detected violation in ground connectivity in low-harmonics mode!");
    }
    ground_connectivity_checker.update(system);

    if (halt) {
        system.bots.clear();
    }

    return halt;
}

bool simulate_all(System& system) {
    while (!system.trace.empty()) {
        if (proceed_timestep(system)) {
            return true;
        }
    }
    return false;
}

bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt) {
    if (m(start_pos) || m(stop_pos)) {
        return false;
    }
    if (start_pos == stop_pos) {
        return true;
    }

    std::deque<Vec3> queue;
    std::unordered_map<Vec3, Vec3, Vec3::hash> parent;
    Matrix blocked = m; // initially Full voxels are blocked.
    queue.push_back(stop_pos);
    blocked(stop_pos) = Full;

    auto n6 = neighbors6();

    while (!queue.empty()) {
        auto p = queue.front(); queue.pop_front();
        for (auto offset : n6) {
            auto n = p + offset;
            if (m.is_in_matrix(n) && !blocked(n)) {
                parent[n] = p;
                if (n == start_pos) {
                    // backtrack.
                    auto cursor = n;
                    do {
                        auto par = parent[cursor];
                        if (trace_opt) {
                            trace_opt->push_back(CommandSMove{par - cursor});
                        }
                        if (trajectory_opt) {
                            trajectory_opt->push_back(par);
                        }
                        cursor = par;
                    } while (cursor != stop_pos);
                    return true;
                }
                queue.push_back(n);
                blocked(n) = Full;
            }
        }
    }

    return false;
}

// vim: set si et sw=4 ts=4:
