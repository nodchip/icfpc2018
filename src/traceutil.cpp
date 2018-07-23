#include "traceutil.h"
#include <unordered_set>
#include <numeric>

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

bool fission_along_x(const std::vector<int>& boundaries, const std::vector<int>& nanobots_at, int N, int R,
        Trace& trace) {

    const int n_positions = boundaries.size();
    ASSERT_RETURN(nanobots_at.size() == n_positions, false);
    ASSERT_RETURN(std::accumulate(nanobots_at.begin(), nanobots_at.end(), 0) == N, false);

    int active = 1;
    int seeds = N - 1;
    int x = 0;
    while (active < n_positions) {
        // (1 + seeds) => nanobots_at + (1 + m)
        for (size_t i = 0; i < active - 1; ++i) {
            trace.push_back(CommandWait{});
        }
        const int m = (1 + seeds) - nanobots_at[active - 1] - 1;
        LOG() << "(1 + " << seeds << ") => (" << nanobots_at[active - 1] << ") + (1 + " << m << ")\n";
        trace.push_back(CommandFission{unitX, m});
        seeds = m;
        ++x;
        ++active;
        // (fast) move.
        while (x < boundaries[active - 1]) {
            for (size_t i = 0; i < active - 1; ++i) {
                trace.push_back(CommandWait{});
            }
            const int step = std::min(15, boundaries[active - 1] - x);
            x += step;
            trace.push_back(CommandSMove{unitX * step});
        }
    }

    return true;
}

bool fission_cube_corner(int w, int h, int d, int R, Vec3 start_pos,
    std::vector<Vec3>& generated_pos,
    System& system) {
    const int tx = min(R - 1, start_pos.x + w);
    const int ty = min(R - 1, start_pos.y + h);
    const int tz = min(R - 1, start_pos.z + d);
    ASSERT_RETURN(start_pos.x < tx, false);
    ASSERT_RETURN(start_pos.y < ty, false);
    ASSERT_RETURN(start_pos.z < tz, false);

    BotID main_bid = system.bid_at(start_pos);

    for (int yy = 0; yy < 2; ++yy) {
        for (int zz = 0; zz < 2; ++zz) {
            for (int xx = 0; xx < 2; ++xx) {
                if (xx + yy + zz > 0) {
                    LOG() << xx << " " << yy << " " << zz << "\n";
                    auto diff = unitX * xx + unitY * yy + unitZ * zz;
                    generated_pos.push_back(start_pos + diff);
                    system.stage(main_bid, CommandFission{diff, 0});
                    system.stage_all_unstaged();
                    system.commit_commands();
                    system.print_detailed();
                }
            }
        }
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

    if (false) {
        fast_move(destination, position, trace);
    } else {
        digging_move(blocked, destination, position, trace);
    }

    return true;
}

bool digging_move(const Matrix& blocked, const Vec3& destination, Vec3& position, Trace& trace) {
    ASSERT_RETURN(!blocked(destination), false);
    ASSERT_RETURN(!blocked(position), false);

    std::vector<Vec3> moves;
    Vec3 work = position;
    const Vec3 dx = (destination.x > work.x) ? unitX : -unitX;
    while (work.x != destination.x) {
        moves.push_back(dx);
        work += dx;
    }
    const Vec3 dy = (destination.y > work.y) ? unitY : -unitY;
    while (work.y != destination.y) {
        moves.push_back(dy);
        work += dy;
    }
    const Vec3 dz = (destination.z > work.z) ? unitZ : -unitZ;
    while (work.z != destination.z) {
        moves.push_back(dz);
        work += dz;
    }

    // dig if required.
    for (size_t i = 0; i < moves.size(); ++i) {
        if (blocked(position + moves[i])) {
            trace.push_back(CommandVoid{moves[i]});
        }
        trace.push_back(CommandSMove{moves[i]});
        position += moves[i];
        if (blocked(position - moves[i])) {
            trace.push_back(CommandFill{-moves[i]});
        }
    }

    return true;
}

}
