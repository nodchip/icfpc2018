// std
#include <iostream>
// 3rd
#include <gtest/gtest.h>
// project
#include "state.h"
#include "trace.h"
#include "nmms.h"
// TODO(peria): Split test for solvers
#include "engines/stupid_solver.h"
#include "engines/stupid_solver_v2.h"
#include "engines/parallel_stupid_solver.h"

TEST(Planning, FastMove) {
    Matrix m(5);
    {
        Trace trace;
        EXPECT_TRUE(fast_manhattan_motion_in_void(m, Vec3(0, 1, 0), Vec3(4, 3, 4), trace));
        EXPECT_EQ(trace.size(), 3);
    }
    {
        Trace trace;
        EXPECT_TRUE(fast_manhattan_motion_in_void(m, Vec3(0, 2, 0), Vec3(4, 2, 4), trace));
        EXPECT_EQ(trace.size(), 2);
    }
    {
        Trace trace;
        EXPECT_TRUE(fast_manhattan_motion_in_void(m, Vec3(0, 2, 4), Vec3(4, 2, 4), trace));
        EXPECT_EQ(trace.size(), 1);
    }
    {
        Trace trace;
        EXPECT_TRUE(fast_manhattan_motion_in_void(m, Vec3(4, 2, 4), Vec3(4, 2, 4), trace));
        EXPECT_EQ(trace.size(), 0);
    }
    { // illegal
        Trace trace;
        EXPECT_FALSE(fast_manhattan_motion_in_void(m, Vec3(6, 2, 4), Vec3(4, 2, 4), trace));
        EXPECT_EQ(trace.size(), 0);
    }
    { // no valid path
        Trace trace;
        m.buf.assign(m.buf.size(), Full);
        EXPECT_FALSE(fast_manhattan_motion_in_void(m, Vec3(0, 0, 0), Vec3(4, 2, 4), trace));
        EXPECT_EQ(trace.size(), 0);
    }
}

TEST(Matrix, LoadAndDumpMatrix) {
    Matrix m("../data/problems/LA001_tgt.mdl");
    EXPECT_EQ(m.R, 20);

    // add some voxels.
    for (int x = 0; x < m.R / 2; ++x) {
        if (x % 2 == 0) {
            m(x, 0, 1) = Voxel::Full;
        } else {
            m(Vec3(x, 1, 0)) = Voxel::Full;
        }
    }

    m.dump("LA001_tgt_modified.mdl");

    // load again to check identity.
    Matrix m2("LA001_tgt_modified.mdl");
    ASSERT_TRUE(m2.is_valid_matrix());
    EXPECT_EQ(m.buf, m2.buf);
}

TEST(Trace, OutputTraceExample) {
    Trace trace;
    trace.push_back(CommandWait{}); // 0
    trace.push_back(CommandFlip{}); // 0
    trace.push_back(CommandSMove{Vec3(0, 0, 10)}); // 0
    trace.push_back(CommandLMove{Vec3(0, 0, 1), Vec3(0, 1, 0)}); // 0
    trace.push_back(CommandFission{Vec3(0, 0, 1), 2}); // 0

    trace.push_back(CommandFill{Vec3(0, 1, 0)}); // 0
    trace.push_back(CommandWait{}); // 1
    trace.push_back(CommandFusionP{Vec3(0, 1, 0)}); // 0
    trace.push_back(CommandFusionS{Vec3(0, -1, 0)}); // 1

    trace.push_back(CommandLMove{Vec3(0, 0, -1), Vec3(0, -1, 0)}); // 0
    trace.push_back(CommandSMove{Vec3(0, 0, -10)}); // 0
    trace.push_back(CommandHalt{}); // 0

    EXPECT_TRUE(trace.output_trace("output.nbt"));

    Trace trace2;
    EXPECT_TRUE(trace2.input_trace("output.nbt"));
    EXPECT_TRUE(trace2.output_trace("output2.nbt"));

    // EXPECT_EQ(trace, trace2);
}

TEST(Commands, FissionAndFusion) {
    System system(4);
    // index to fission out
    int m = 2;

    // Fission
    system.trace.push_back(CommandFission{Vec3 {0, 0, 1}, m});
    EXPECT_FALSE(system.proceed_timestep());

    // system.print_detailed();

    ASSERT_EQ(system.bots.size(), 2);
    EXPECT_EQ(system.bots[0].seeds.size(), 18 - m);
    EXPECT_EQ(system.bots[1].seeds.size(), m);
    EXPECT_EQ(system.trace.size(), 0);

    // Fusion
    system.trace.push_back(CommandFusionP{Vec3 {0, 0, 1}});
    system.trace.push_back(CommandFusionS{Vec3 {0, 0, -1}});
    EXPECT_FALSE(system.proceed_timestep());

    // system.print_detailed();

    EXPECT_EQ(system.bots.size(), 1);
    EXPECT_EQ(system.bots[0].seeds.size(), 19);
}

TEST(System, StupidSolver) {
    Matrix m("../data/problems/LA001_tgt.mdl");
    auto trace = stupid_solver(m);

    State state(m);
    EXPECT_FALSE(state.is_finished());
    state.simulate(trace);

    EXPECT_TRUE(state.is_finished());
}

TEST(System, StupidSolverV2) {
    Matrix m("../data/problems/LA001_tgt.mdl");
    auto trace = stupid_solver_v2(m);

    State state(m);
    EXPECT_FALSE(state.is_finished());
    state.simulate(trace);

    EXPECT_TRUE(state.is_finished());
}

TEST(System, ParallelStupidSolver) {
    Matrix m("../data/problems/LA001_tgt.mdl");
    auto trace = parallel_stupid_solver(m);

    State state(m);
    EXPECT_FALSE(state.is_finished());
    state.simulate(trace);

    EXPECT_TRUE(state.is_finished());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
// vim: set si et sw=4 ts=4:
