#include "parallel_optimized_v2.h"

#include "engines/naive_converter.h"
#include "engine.h"
#include "nmms.h"
#include "system.h"
#include "state.h"
#include "log.h"
#include "naive_move.h"

namespace {

static const Vec3 unitX(1, 0, 0);
static const Vec3 unitY(0, 1, 0);
static const Vec3 unitZ(0, 0, 1);

Trace single_stupid_solver(const System& system, const Matrix& tgt_matrix,
                           const Vec3& lower_bound, const Vec3& upper_bound,
                           Vec3& position) {
    // calculate bounding box
    Vec3 lower = upper_bound - Vec3(1, 1, 1);
    Vec3 upper = lower_bound;
    for (int x = lower_bound.x; x < upper_bound.x; ++x) {
        for (int y = lower_bound.y; y < upper_bound.y; ++y) {
            for (int z = lower_bound.z; z < upper_bound.z; ++z) {
                if (tgt_matrix(Vec3(x, y, z))) {
                    lower.x = std::min(lower.x, x);
                    lower.y = std::min(lower.y, y);
                    lower.z = std::min(lower.z, z);
                    upper.x = std::max(upper.x, x + 1);
                    upper.y = std::max(upper.y, y + 1);
                    upper.z = std::max(upper.z, z + 1);
                }
            }
        }
    }
    if (lower.x >= upper.x) {
        return {};
    }

    // print
    Trace trace;
    naive_move(lower, position, trace, EFofReduction);
    int dx = -1;
    int dz = -1;
    for (int y = lower.y + 1; y <= upper.y; ++y) {
        dz *= -1;
        trace.push_back(CommandSMove{unitY});
        position += unitY;
        for (int z = lower.z; z < upper.z; ++z) {
            dx *= -1;
            if (z > lower.z) {
                trace.push_back(CommandSMove{Vec3(0, 0, dz)});
                position += Vec3(0, 0, dz);
            }
            for (int x = lower.x; x < upper.x; ++x) {
                if (x > lower.x) {
                    trace.push_back(CommandSMove{Vec3(dx, 0, 0)});
                    position += Vec3(dx, 0, 0);
                }
                if (tgt_matrix(position - unitY)) {
                    trace.push_back(CommandFill{-unitY});
                }
            }
        }
    }

    return trace;
}
  
Trace optimize_stupid_trace(const Trace& trace) {
    std::list<Command> commands(trace.begin(), trace.end());

    // merge 2 or 3 Fills sandwiching SMove(s)
    if (commands.size() > 2) {
        auto mergeable = [](const Vec3& first_fill_direction,
                            const Vec3& move_direction,
                            const Vec3& second_fill_direction) {
            return first_fill_direction == second_fill_direction &&
                mlen(move_direction) == 1 &&
                is_valid_nd(first_fill_direction + move_direction) &&
                !is_valid_ld(first_fill_direction + move_direction);
        };
        for (auto third = std::next(commands.begin(), 2); third != commands.end(); ++third) {
            const auto second = std::prev(third);
            const auto first = std::prev(second);
            const auto* first_cmd = boost::get<CommandFill>(&*first);
            const auto* second_cmd = boost::get<CommandSMove>(&*second);
            const auto* third_cmd = boost::get<CommandFill>(&*third);
            if (!first_cmd || !second_cmd || !third_cmd) {
                continue;
            }
            const auto fill_direction = first_cmd->nd;
            const auto move_direction = second_cmd->lld;
            if (!mergeable(fill_direction, move_direction, third_cmd->nd)) {
                continue;
            }
            *first = CommandSMove{move_direction};
            *second = CommandFill{fill_direction - move_direction};

            if (std::next(third) != commands.end() && std::next(third, 2) != commands.end()) {
                const auto fourth = std::next(third);
                const auto fifth = std::next(fourth);
                const auto* fourth_cmd = boost::get<CommandSMove>(&*fourth);
                const auto* fifth_cmd = boost::get<CommandFill>(&*fifth);
                if (!fourth_cmd || !fifth_cmd || fourth_cmd->lld != move_direction) {
                    continue;
                }
                if (!mergeable(fill_direction, move_direction, fifth_cmd->nd)) {
                    continue;
                }
                *fourth = CommandFill{fill_direction + move_direction};
                *fifth = CommandSMove{move_direction};
            }
        }
    }

    auto commands_to_trace = [&] {
        Trace t;
        std::copy(commands.begin(), commands.end(), std::back_inserter(t));
        return t;
    };

    // reduce unnecessary turns
    int removed_turns = 0;
    while (true) {
        if (commands.size() < 3) break;
        bool updated = false;
        auto middle = std::next(commands.begin());
        while (std::next(middle) != commands.end()) {
            if (middle == commands.begin()) {
                ++middle;
                continue;
            }
            const auto prev = std::prev(middle);
            const auto next = std::next(middle);
            const auto* prev_cmd = boost::get<CommandSMove>(&*prev);
            const auto* middle_cmd = boost::get<CommandSMove>(&*middle);
            const auto* next_cmd = boost::get<CommandSMove>(&*next);
            if (!prev_cmd || !middle_cmd || !next_cmd) {
                ++middle;
                continue;
            }
            if (mlen(middle_cmd->lld) == 1 && prev_cmd->lld == -next_cmd->lld) {
                commands.erase(prev);
                commands.erase(next);
                updated = true;
                ++removed_turns; 
            } else {
                ++middle;
            }
        }
        if (!updated) break;
    }
    if (false) {
        Trace t = commands_to_trace();
        t.print_detailed();
        LOG() << "removed turns = " << removed_turns << "\n";
    }

    // merge straight moves
    int removed_straights = 0;
    if (commands.size() > 1) {
        auto first = commands.begin();
        while (std::next(first) != commands.end()) {
            const auto second = std::next(first);
            const auto* first_cmd = boost::get<CommandSMove>(&*first);
            const auto* second_cmd = boost::get<CommandSMove>(&*second);
            if (!first_cmd || !second_cmd) {
                ++first;
                continue;
            }
            if (is_valid_long_ld(first_cmd->lld + second_cmd->lld)) {
                *first = CommandSMove{first_cmd->lld + second_cmd->lld};
                commands.erase(second);
                ++removed_straights;
            } else {
                ++first;
            }
        }
    }
    if (false) {
        Trace t = commands_to_trace();
        t.print_detailed();
        LOG() << "removed straights = " << removed_straights << "\n";
    }

    // convert SMove to LMove
    int converted_smoves = 0;
    if (commands.size() > 1) {
        auto first = commands.begin();
        while (std::next(first) != commands.end()) {
            const auto second = std::next(first);
            const auto* first_cmd = boost::get<CommandSMove>(&*first);
            const auto* second_cmd = boost::get<CommandSMove>(&*second);
            if (!first_cmd || !second_cmd) {
                ++first;
                continue;
            }
            auto combine = first_cmd->lld + second_cmd->lld;
            if (is_valid_short_ld(first_cmd->lld) && is_valid_short_ld(second_cmd->lld) && !is_valid_long_ld(combine)) {
                *first = CommandLMove{first_cmd->lld, second_cmd->lld};
                commands.erase(second);
                ++converted_smoves;
            } else {
                ++first;
            }
        }
    }
    if (false) {
        Trace t = commands_to_trace();
        t.print_detailed();
        LOG() << "SMove -> LMove converted = " << converted_smoves << "\n";
    }


    Trace optimized_trace;
    optimized_trace.insert(optimized_trace.end(), commands.begin(), commands.end());
    return optimized_trace;
}

pair<long long int, vector<int>> sepbound(const vector<long long int> &cellnum, const long long int lim, const int num){
  vector<int> out;
  out.push_back(0);
  out.push_back(1);

  long long int cnt = 0;
  long long int maxcnt = 0;

  bool boundnext = false;
  for(int x=1; x<=cellnum.size(); ++x){
    const int osiz = out.size();
    if(boundnext && osiz < num -1){
      out.push_back(x);
      if(cnt > maxcnt){
	maxcnt = cnt;
      }
      cnt = 0;
      boundnext = false;
    }else if(num + 1 - osiz == cellnum.size() - x + 1){
      out.push_back(x);
      if(cnt > maxcnt){
	maxcnt = cnt;
      }
      cnt = 0;
      boundnext = false;
    }
    if(x<cellnum.size()){
      cnt += cellnum[x];
      if(cnt > lim){
	boundnext = true;
      }
    }
  }
  
  return pair<long long int, vector<int>>(maxcnt, out);
}

long long int getcellnumber(const Matrix &matrix, const int x, const int zlower, const int zupper){
  long long int output = 0;
  for(int y=0;y<matrix.R;++y){
    for(int z=zlower;z<zupper;++z){
      if(matrix(x,y,z))
	++output;
    }
  }
  return output;
}

  
Trace solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) {
    if (problem_type == ProblemType::Disassembly) {
        return NaiveConverter::reverse(solver)(problem_type, src_matrix, tgt_matrix);
    } else if (problem_type == ProblemType::Reassembly) {
        return NaiveConverter::concatenate(NaiveConverter::reverse(solver), solver)(problem_type, src_matrix, tgt_matrix);
    }
    ASSERT_RETURN(problem_type == ProblemType::Assembly, Trace());

    System system(tgt_matrix.R);
    Trace trace;

    // spread nanobots
    ASSERT(system.bots.size() == 1);
    const int R = system.matrix.R;
    const int N = std::min<int>(system.bots.size() + system.bots[0].seeds.size(), R);

    std::vector<int> boundaries;
    std::vector<long long int> cellnum;
    long long int approx_index = 0;
    for(int x=0; x<R; ++x){
      const long long int cnum = getcellnumber(tgt_matrix, x, 0, R);
      cellnum.push_back(cnum);
      approx_index += cnum;
    }

    // dist
    approx_index = (approx_index / N) + 1;
    long long int maxcnt = 1145141919;
    int imax = 256;
    for(int i = 1; i<= imax; ++i){
      pair<long long int, vector<int>> result = sepbound(cellnum, approx_index * i /imax, N);
      if(maxcnt > result.first){
	boundaries = result.second;
	maxcnt = result.first;
      }
    }
    
    ASSERT(system.bots[0].pos == Vec3(0, 0, 0));
    std::vector<Vec3> positions(N, system.bots[0].pos);
    int num_active_nanobots = 1;
    while (positions[N - 1].x < boundaries[N - 1]) {
        const int index = num_active_nanobots - 1;
        for (int i = 0; i < index; ++i) {
            trace.push_back(CommandWait{});
        }
        if (positions[index].x == boundaries[index]) {
            trace.push_back(CommandFission{unitX, N - ++num_active_nanobots});
            positions[index + 1] = positions[index] + unitX;
        } else {
            trace.push_back(CommandSMove{unitX});
            positions[index] += unitX;
        }
    }
    auto shadow_positions = positions;

    // run each nanobot
    int highest = 0;
    std::vector<Trace> traces;
    traces.push_back({});
    for (int i = 1; i < N; ++i) {
        const auto stupid_trace = single_stupid_solver(
            system, tgt_matrix,
            Vec3(boundaries[i], 0, 0),
            Vec3(boundaries[i + 1], R, R),
            positions[i]);
        traces.push_back(optimize_stupid_trace(stupid_trace));
        highest = std::max(highest, positions[i].y);
    }
    std::size_t max_trace_size = 0;
    for (int i = 1; i < N; ++i) {
        naive_move(Vec3(positions[i].x, highest, positions[i].z),
             positions[i], traces[i], EFofReduction);
        max_trace_size = std::max(max_trace_size, traces[i].size());
    }

    // merge
    auto to_index = [R](const Vec3& p) {
        return p.x + p.y * R + p.z * R * R;
    };
    bool high_harmonics = false;
    int num_full = 0;
    const int ground = R * R * R;
    std::vector<bool> voxels(ground);
    UnionFind union_find(ground + 1);
    for (std::size_t t = 0; t < max_trace_size; ++t) {
        const auto flipper = trace.size();
        for (int i = 0; i < N; ++i) {
            if (t < traces[i].size()) {
                trace.push_back(traces[i][t]);
                if (auto* cmd = boost::get<CommandSMove>(&trace.back())) {
                    shadow_positions[i] += cmd->lld;
                } else if (auto* cmd = boost::get<CommandLMove>(&trace.back())) {
                    shadow_positions[i] += cmd->sld1 + cmd->sld2;
                } else if (auto* cmd = boost::get<CommandFill>(&trace.back())) {
                    const auto p = shadow_positions[i] + cmd->nd;
                    const auto index = to_index(p);
                    ++num_full;
                    voxels[index] = true;
                    if (p.y == 0) {
                        union_find.unionSet(ground, index);
                    } else {
                        for (const auto d : neighbors6) {
                            const auto neighbor = to_index(p + d);
                            if (voxels[neighbor]) {
                                union_find.unionSet(index, to_index(p + d));
                            }
                        }
                    }
                }
            } else {
                trace.push_back(CommandWait{});
            }
        }

        const bool grounded = union_find.size(ground) == num_full + 1;
        if (high_harmonics == grounded) {
            trace[flipper] = CommandFlip{};
            high_harmonics = !grounded;
        }
    }
    ASSERT(!high_harmonics);
    ASSERT(union_find.size(ground) == num_full + 1);

    // go home
    for (int i = N - 1; i > 0; --i) {
        Trace trace_to_join;
        naive_move(positions[i - 1], positions[i], trace_to_join, EFofReduction);
        const auto* cmd = boost::get<CommandSMove>(&trace_to_join.back());
        ASSERT(cmd);
        Vec3 unit;
        if (cmd->lld[0]) {
            unit = (cmd->lld[0] > 0) ? unitX : -unitX;
        } else if (cmd->lld[1]) {
            unit = (cmd->lld[1] > 0) ? unitY : -unitY;
        } else {
            unit = (cmd->lld[2] > 0) ? unitZ : -unitZ;
        }
        positions[i] -= unit;
        if (mlen(cmd->lld) == 1) {
            trace_to_join.pop_back();
        } else {
            trace_to_join.back() = CommandSMove{cmd->lld - unit};
        }
        for (std::size_t t = 0; t < trace_to_join.size(); ++t) {
            for (int j = 0; j < i; ++j) {
                trace.push_back(CommandWait{});
            }
            trace.push_back(trace_to_join[t]);
        }
        for (int j = 0; j < i - 1; ++j) {
            trace.push_back(CommandWait{});
        }
        trace.push_back(CommandFusionP{positions[i] - positions[i - 1]});
        trace.push_back(CommandFusionS{positions[i - 1] - positions[i]});
    }
    naive_move(Vec3(0, 0, 0), positions[0], trace, EFofReduction);

    // finalize at the origin pos.
    trace.push_back(CommandHalt{});
    return trace;
}

}  // namespace

REGISTER_ENGINE(parallel_optimized_v2, solver);
