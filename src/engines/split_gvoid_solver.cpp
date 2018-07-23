#include "naive_converter.h"

#include "engine.h"
#include "nmms.h"
#include "system.h"
#include "state.h"
#include "traceutil.h"
#include "log.h"

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

Trace split_gvoid_solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) {
    if (problem_type == ProblemType::Disassembly) {
        return NaiveConverter::reverse(split_gvoid_solver)(problem_type, src_matrix, tgt_matrix);
    } else if (problem_type == ProblemType::Reassembly) {
        return NaiveConverter::concatenate(NaiveConverter::reverse(split_gvoid_solver), split_gvoid_solver)(problem_type, src_matrix, tgt_matrix);
    }
    ASSERT_RETURN(problem_type == ProblemType::Assembly, Trace());

    State state(src_matrix, tgt_matrix);
    System& system = state.system;
    Trace trace;

    // spread nanobots
    ASSERT(system.bots.size() == 1);
    const int R = system.matrix.R;
    const int N = system.bots.size() + system.bots[0].seeds.size();
    const int N8 = N / 8; // grouping.
    ASSERT_EQ(N8 * 8, N);
    std::vector<int> positions;
    std::vector<int> nanobots_at(N8, 8);
    std::vector<int> boundaries; // size N8 + 1.
    for (int i = 0; i < N8; ++i) {
        positions.push_back(R * i / (N8 + 1));
        boundaries.push_back(R * i / (N8 + 1));
    }
    boundaries.push_back(R);

    NTraceUtil::fission_along_x(positions, nanobots_at, N, R, trace);
    state.append_simulate_partial(trace);
    trace.clear();
    system.print_detailed();

    LOG() << "expand...........................\n";

    // expand to cube.
    auto seed_bot_positions = NTraceUtil::bot_positions(system);
    std::vector<std::vector<Vec3>> corners_pos(N8);
    std::vector<std::vector<BotID>> corners_bid(N8);
    for (int i = 0; i < N8; ++i) {
        NTraceUtil::fission_cube_corner(30, 30, 30, R, seed_bot_positions[i], corners_pos[i], system);
        corners_pos[i].push_back(seed_bot_positions[i]);
        for (auto& p : corners_pos[i]) {
            corners_bid[i].push_back(system.bid_at(p));
        }
    }

    system.trace.push_back(CommandHalt{});
    return system.trace;
}

REGISTER_ENGINE(split_gvoid, split_gvoid_solver);
// vim: set si et sw=4 ts=4:

