#include "nmms.h"
// std
#include <cstdio>
// 3rd
#include <gtest/gtest.h>

TEST(Matrix, LoadAndDumpMatrix) {
    Matrix m = load_model("../problems/LA001_tgt.mdl");
    ASSERT_TRUE(m);
    std::printf("Model R=%d\n", m.R);
    EXPECT_EQ(m.R, 20);

    // add some voxels.
    for (int x = 0; x < m.R / 2; ++x) {
        if (x % 2 == 0) {
            m(x, 0, 1) = 1;
        } else {
            m(Vec3(x, 1, 0)) = 1;
        }
    }

    dump_model("LA001_tgt_modified.mdl", m);

    // load again to check identity.
    Matrix m2 = load_model("LA001_tgt_modified.mdl");
    ASSERT_TRUE(m2);
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

    EXPECT_TRUE(output_trace("output.nbt", trace));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv); 
    return RUN_ALL_TESTS();
}
// vim: set si et sw=4 ts=4:
