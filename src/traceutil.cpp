#include "traceutil.h"
#include <unordered_set>

#include "system.h"
#include "state.h"
#include "command.h"
#include "nmms.h"
#include "log.h"

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

namespace NTraceUtil {

std::vector<Vec3> create_x_linear_positions(const Matrix& blocked, int n_nanobots) {
    std::vector<Vec3> res;
    const int R = blocked.R;
    ASSERT_RETURN(n_nanobots < R, res);

    for (int i = 0; i < n_nanobots; ++i) {
        res.emplace_back(i, 0, 0);
    }

    return res;
}

bool fusion_x_linear_positions_to_first_pos(const std::vector<Vec3>& positions, Trace& trace) {
    ASSERT_RETURN(!positions.empty(), false);
    const int N = positions.size();
    int active = N;

    while (active > 1) {
        // wait wait wait FusionP FusionS
        // wait wait FusionP FusionS
        // ..
        for (int i = 0; i < active; ++i) {
            if (i < active - 2) {
                trace.push_back(CommandWait{});
            } else if (i == active - 2) {
                auto nd = positions[i + 1] - positions[i];
                ASSERT_RETURN(is_valid_nd(nd), false);
                trace.push_back(CommandFusionP{nd});
            } else {
                auto nd = positions[i - 1] - positions[i];
                ASSERT_RETURN(is_valid_nd(nd), false);
                trace.push_back(CommandFusionS{nd});
            }
        }
        --active;
    }

    return true;
}

bool fission_x_2_linear_positions(Vec3 start_pos, int N, int R, std::vector<Vec3>& id_to_pos, Trace& trace) {
    id_to_pos.clear();
    id_to_pos.push_back(start_pos + unitX * N);

    int active = 1;
    while (active < N) {
        if (active == 1) {
            trace.push_back(CommandSMove{unitX});
        } else {
            trace.push_back(CommandSMove{unitX * 2});
        }
        for (int i = 0; i < active - 1; ++i) {
            trace.push_back(CommandWait{});
        }
        //
        trace.push_back(CommandFission{-unitX, 1});
        id_to_pos.push_back(start_pos + unitX * (active - 1));
        for (int i = 0; i < active - 1; ++i) {
            trace.push_back(CommandWait{});
        }
        ++active;
    }

    return true;
}

bool fission_x_2by2_linear_positions(Vec3 start_pos, int N, int R, std::vector<Vec3>& id_to_pos, Trace& trace) {
    Matrix msrc(R), mtgt(R);
    State state(msrc, mtgt);
    System& system = state.system;
    system.set_verbose(false);

    // (2*i) x (0, 2) x (0, 2)
    while (system.bots.size() < N) {
        system.stage(system.bots[0], CommandFission{unitY, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();

        system.stage(system.bots[0], CommandFission{unitY + unitZ, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();

        system.stage(system.bots[0], CommandFission{unitZ, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();

        if (system.bots.size() == N) {
            break;
        }

        system.stage(system.bots[0], CommandSMove{unitX});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();

        system.stage(system.bots[0], CommandFission{-unitX, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();

        system.stage(system.bots[0], CommandSMove{unitX});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        //system.print_detailed();
    }

    id_to_pos.resize(N);
    for (size_t i = 0; i < system.bots.size(); ++i) {
        id_to_pos[i] = system.bots[i].pos;
    }

    trace = system.trace;
    return true;
}

std::vector<Vec3> bot_positions(const System& system) {
    std::vector<Vec3> res;
    for (auto& bot : system.bots) {
        res.push_back(bot.pos);
    }
    return res;
}

bool merge_traces(const std::vector<Trace>& traces, Trace& trace) {
    size_t len = 0;
    for (auto& t : traces) {
        len = std::max(t.size(), len);
    }
    LOG() << "MERGE " << traces.size() << " traces. maxlen = " << len << std::endl;

    for (size_t i = 0; i < len; ++i) {
        for (size_t b = 0; b < traces.size(); ++b) {
            if (i < traces[b].size()) {
                Command cmd = traces[b][i];
                ASSERT(cmd.type() != typeid(CommandHalt));
                ASSERT(cmd.type() != typeid(CommandFusionS));
                ASSERT(cmd.type() != typeid(CommandFusionP));
                ASSERT(cmd.type() != typeid(CommandFission));
                trace.push_back(cmd);
            } else {
                trace.push_back(CommandWait{});
            }
        }
    }

    return true;
}


#if 0
bool multibot_merge_conservative(const Matrix& blocked, const std::vector<Vec3>& start_positions,
        const std::vector<Vec3>& final_positions, Trace& trace) {
    const size_t N = start_positions.size();
    std::vector<Trace> traces(N);

    Matrix work = blocked;

    for (size_t i = 0; i < N; ++i) {
        // prevent from blocking other bot's goal.
        for (size_t j = 0; j < N; ++j) {
            work(final_positions[j]) = i == j ? Voxel::Void : Voxel::Full;
            work(start_positions[j]) = i == j ? Voxel::Void : Voxel::Full;
        }

        std::vector<Vec3> trajectory;
        ASSERT_RETURN(bfs_shortest_in_void(work,
             start_positions[i], final_positions[i], &traces[i], &trajectory), false);

        for (auto& p : trajectory) {
            ASSERT_RETURN(work(p) == Voxel::Void, false);
            work(p) = Voxel::Full;
        }

        LOG() << "bot " << i << " OK\n";
    }

    ASSERT_RETURN(merge_traces(traces, trace), false);

    return true;
}
#endif

bool move_evacuated_multibots(const Matrix& blocked, const std::vector<Vec3>& start_positions,
        const std::vector<Vec3>& final_positions, Trace& trace) {
    const size_t N = start_positions.size();
    const int R = blocked.R;

    std::vector<Vec3> positions = start_positions;

    const Region core = core_region(blocked.R);

    for (size_t i = 0; i < N; ++i) {
        ASSERT_RETURN(blocked.is_in_matrix(positions[i]), false);
        ASSERT_RETURN(!blocked(positions[i]), false);
        ASSERT_RETURN(!core.is_in_region(positions[i]), false);
    }

    std::vector<int> moving(N, 1);
    for (size_t i = 0; i < N; ++i) {
        moving[i] = positions[i] != final_positions[i] ? 1 : 0;
    }

    auto is_blocked_by_others = [&](int idx, Vec3 pos) {
        for (size_t i = 0; i < N; ++i) {
            if (i != idx && positions[i] == pos) {
                return true;
            }
        }
        return false;
    };

    int n_moving = N;
    int n_steps = 0;
    while (n_moving > 0) {
        std::unordered_set<Vec3> reserved;
        std::vector<Command> resolved(N, CommandWait{});
        int num_moved = 0;

        // stage moves. greedy: younger bot first.
        for (size_t i = 0; i < N; ++i) {
            if (moving[i]) {
                int best_d2 = length2({R, R, R});
                int best_n = -1;
                for (int n = 0; n < 6; ++n) {
                    auto c = positions[i] + neighbors6[n];
                    if (!core.is_in_region(c) && blocked.is_in_matrix(c)) {
                        int d2 = length2(c - final_positions[i]);
                        if (d2 < best_d2) {
                            ASSERT_RETURN(!blocked(c), false);
                            if (reserved.find(c) == reserved.end()) {
                                if (!is_blocked_by_others(i, c)) {
                                    best_d2 = d2;
                                    best_n = n;
                                }
                            }
                        }
                    }
                }
                if (best_n >= 0) {
                    Vec3 motion = neighbors6[best_n];
                    auto target = positions[i] + motion;

                    // greedy.
                    resolved[i] = CommandSMove{motion};
                    positions[i] = target;
                    reserved.insert(target);
                    ++num_moved;

                    if (target == final_positions[i]) {
                        moving[i] = 0;
                        --n_moving;
                    }
                }
            }
        }
        ASSERT_RETURN(num_moved > 0, false); // deadlock

        // commit.
        for (size_t i = 0; i < N; ++i) {
            //LOG() << "BOT " << i << " : " << resolved[i] << " pos " << positions[i] << " tgt " << final_positions[i]
            //    << (moving[i] ? "[o]" : "[ ]") << std::endl;

            trace.push_back(resolved[i]);
        }
        ++n_steps;
    }

    LOG() << "timesteps " << n_steps << std::endl;

    return true;
}

bool digging_evacuate(const Matrix& blocked, Vec3& position, Trace& trace) {
    const int R = blocked.R;
    const int axis[6] = {0, 0, 1, 1, 2, 2};
    const int target[6] = {R - 1, 0, R - 1, 0, R - 1};

    int best_d = R;
    int best_i = 0;
    for (int i = 0; i < 6; ++i) {
        int d = std::abs(position[axis[i]] - target[i]);
        if (d < best_d) {
            best_d = d;
            best_i = i;
        }
    }

    Vec3 destination = position;
    destination[axis[best_i]] = target[best_i];

    // TODO: dig.
    fast_move(destination, position, trace);

    return true;
}

}
