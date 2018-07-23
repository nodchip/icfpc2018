
#include "parallel_optimized_v3.h"

#include "engines/naive_converter.h"
#include "engine.h"
#include "nmms.h"
#include "system.h"
#include "state.h"
#include "traceutil.h"
#include "log.h"

Trace solver_v3(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix);

#define REP(n) for (int k = 0; k < (n); ++k)

namespace {

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
const Vec3 unitZ(0, 0, 1);


Trace solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) {
    ASSERT_RETURN(problem_type == ProblemType::Assembly, Trace());

    const int R = tgt_matrix.R;
    // find a cube.
    Region cube({0,0,0},{0,0,0});
    bool cube_found = false;
    [&] {
        int size = 30;
        for (; size > 10; size -= 2) {
            for (int z = 0; z < R - size - 1; ++z) {
                for (int y = 0; y < R - size - 1; ++y) {
                    for (int x = 0; x < R - size - 1; ++x) {
                        bool is_cube = true;
                        for (int zz = z; zz <= z + size; ++zz) {
                            for (int yy = y; yy <= y + size; ++yy) {
                                for (int xx = x; xx <= x + size; ++xx) {
                                    if (!tgt_matrix(xx, yy, zz)) {
                                        is_cube = false;
                                        break;
                                    }
                                }
                            }
                        }
                        if (is_cube) {
                            cube.c1 = Vec3(x, y, z);
                            cube.c2 = Vec3(x + size, y + size, z + size);
                            cube_found = true;
                            return;
                        }
                    }
                }
            }
        }
    }();

    ASSERT_RETURN(cube_found, Trace {});

    const int cube_size = (cube.c2 - cube.c1)[0];
    LOG() << cube.c1 << "~" << cube.c2 << " size " << (cube_size) << "\n";

    // a matrix without the cube.
    Matrix erased_matrix = tgt_matrix;
    {
        int x = cube.c1.x;
        int y = cube.c1.y;
        int z = cube.c1.z;
        for (int zz = z; zz <= z + cube_size; ++zz) {
            for (int yy = y; yy <= y + cube_size; ++yy) {
                for (int xx = x; xx <= x + cube_size; ++xx) {
                    erased_matrix(xx, yy, zz) = Voxel::Void;
                }
            }
        }
    }

    // use another solver for the modified problem.
    Trace trace = solver_v3(problem_type, src_matrix, erased_matrix);
    trace.pop_back(); // halt

    State state(src_matrix, tgt_matrix);
    state.append_simulate_partial(trace);
    trace.clear();

    state.system.set_verbose(true);

    auto infopoint = [&](std::string s) {
        LOG() << " =============== " << s << "\n";
        if (!trace.empty()) {
            state.append_simulate_partial(trace);
            trace.clear();
        }
    };

    infopoint("begin");

    // go to cube (x, 0, z)
    {
        Vec3 p(0, 0, 0);
        NTraceUtil::digging_move(erased_matrix, Vec3(cube.c1.x, 0, cube.c1.z), p, trace);
    }
    infopoint("cube bottom");

    // expand
    // B4 B3
    // B1 B2

    // 1->2
    int K = 39;
    trace.push_back(CommandVoid{unitX});
    //
    trace.push_back(CommandFission{unitX, K - 2});

    // 2->3
    trace.push_back(CommandWait{});
    trace.push_back(CommandVoid{unitZ});
    //
    trace.push_back(CommandWait{});
    trace.push_back(CommandFission{unitZ, K - 4});

    // 2->4
    trace.push_back(CommandWait{});
    trace.push_back(CommandWait{});
    trace.push_back(CommandVoid{-unitX});
    //
    trace.push_back(CommandWait{});
    trace.push_back(CommandWait{});
    trace.push_back(CommandFission{-unitX, K - 6});

    infopoint("expand");

    // to corner.
    for (int i = 0; i < cube_size - 1; ++i) {
        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandVoid{unitX}); // 2
        trace.push_back(CommandVoid{unitX + unitZ}); // 3
        trace.push_back(CommandVoid{unitZ}); // 4

        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandWait{}); // 2
        trace.push_back(CommandVoid{unitX}); // 3
        trace.push_back(CommandWait{}); // 4

        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandWait{}); // 2
        trace.push_back(CommandVoid{unitZ}); // 3
        trace.push_back(CommandWait{}); // 4
        
        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandSMove{unitX}); // 2
        trace.push_back(CommandLMove{unitX, unitZ}); // 3
        trace.push_back(CommandSMove{unitZ}); // 4
    }
    infopoint("corner");

    // go up.
    for (int i = 0; i < cube.c1.y; ++i) {
        REP(4) trace.push_back(CommandVoid{unitY}); // 1-4
        REP(4) trace.push_back(CommandSMove{unitY}); // 1-4
    }
    infopoint("up");

    // clear right ahead for fission.
    trace.push_back(CommandVoid{unitY}); // 1
    trace.push_back(CommandVoid{unitY}); // 2
    trace.push_back(CommandVoid{unitY}); // 3
    trace.push_back(CommandVoid{unitY}); // 4

    // another 4 points
    trace.push_back(CommandFission{unitY, 0}); // 1 -> 5
    trace.push_back(CommandFission{unitY, 0}); // 2 -> 6
    trace.push_back(CommandFission{unitY, 0}); // 3 -> 7
    trace.push_back(CommandFission{unitY, 0}); // 4 -> 8

    infopoint("up fission");

    // go up.
    for (int i = 0; i < cube_size - 1; ++i) {
        REP(4) trace.push_back(CommandWait{}); // 1-4
        REP(4) trace.push_back(CommandVoid{unitY}); // 5-8

        REP(4) trace.push_back(CommandWait{}); // 1-4
        REP(4) trace.push_back(CommandSMove{unitY}); // 5-8
    }

    infopoint("reach cube");

    {
        Vec3 pos[8] = {
            Vec3(cube.c1.x, cube.c1.y, cube.c1.z),
            Vec3(cube.c2.x, cube.c1.y, cube.c1.z),
            Vec3(cube.c2.x, cube.c1.y, cube.c2.z),
            Vec3(cube.c1.x, cube.c1.y, cube.c2.z),
            Vec3(cube.c1.x, cube.c2.y, cube.c2.z),
            Vec3(cube.c2.x, cube.c2.y, cube.c2.z),
            Vec3(cube.c2.x, cube.c2.y, cube.c1.z),
            Vec3(cube.c1.x, cube.c2.y, cube.c1.z),
        };
        Vec3 inpos[8] = {
            Vec3(cube.c1.x+1, cube.c1.y, cube.c1.z+1),
            Vec3(cube.c2.x-1, cube.c1.y, cube.c1.z+1),
            Vec3(cube.c2.x-1, cube.c1.y, cube.c2.z-1),
            Vec3(cube.c1.x+1, cube.c1.y, cube.c2.z-1),
            Vec3(cube.c1.x+1, cube.c2.y, cube.c2.z-1),
            Vec3(cube.c2.x-1, cube.c2.y, cube.c2.z-1),
            Vec3(cube.c2.x-1, cube.c2.y, cube.c1.z+1),
            Vec3(cube.c1.x+1, cube.c2.y, cube.c1.z+1),
        };
        // cube
        trace.push_back(CommandGFill{inpos[0] - pos[0], inpos[5] - inpos[0]});
        trace.push_back(CommandGFill{inpos[1] - pos[1], inpos[4] - inpos[1]});
        trace.push_back(CommandGFill{inpos[2] - pos[2], inpos[7] - inpos[2]});
        trace.push_back(CommandGFill{inpos[3] - pos[3], inpos[6] - inpos[3]});
        trace.push_back(CommandGFill{inpos[4] - pos[4], inpos[1] - inpos[4]});
        trace.push_back(CommandGFill{inpos[5] - pos[5], inpos[0] - inpos[5]});
        trace.push_back(CommandGFill{inpos[6] - pos[6], inpos[3] - inpos[6]});
        trace.push_back(CommandGFill{inpos[7] - pos[7], inpos[2] - inpos[7]});
        infopoint("cube");

        // face XY : OK
        trace.push_back(CommandGFill{ unitX, (pos[6] - unitX) - (pos[0] + unitX)}); // a
        trace.push_back(CommandGFill{-unitX, (pos[7] + unitX) - (pos[1] - unitX)}); // a
        trace.push_back(CommandGFill{-unitX, (pos[4] + unitX) - (pos[2] - unitX)}); // b
        trace.push_back(CommandGFill{ unitX, (pos[5] - unitX) - (pos[3] + unitX)}); // b
        trace.push_back(CommandGFill{ unitX, (pos[2] - unitX) - (pos[4] + unitX)}); // a
        trace.push_back(CommandGFill{-unitX, (pos[3] + unitX) - (pos[5] - unitX)}); // a
        trace.push_back(CommandGFill{-unitX, (pos[0] + unitX) - (pos[6] - unitX)}); // b
        trace.push_back(CommandGFill{ unitX, (pos[1] - unitX) - (pos[7] + unitX)}); // b
        infopoint("face XY");

        // face ZY
        trace.push_back(CommandGFill{ unitZ, (pos[4] - unitZ) - (pos[0] + unitZ)}); // a
        trace.push_back(CommandGFill{ unitZ, (pos[5] - unitZ) - (pos[1] + unitZ)}); // b
        trace.push_back(CommandGFill{-unitZ, (pos[6] + unitZ) - (pos[2] - unitZ)}); // a
        trace.push_back(CommandGFill{-unitZ, (pos[7] + unitZ) - (pos[3] - unitZ)}); // b
        trace.push_back(CommandGFill{-unitZ, (pos[0] + unitZ) - (pos[4] - unitZ)}); // a
        trace.push_back(CommandGFill{-unitZ, (pos[1] + unitZ) - (pos[5] - unitZ)}); // b
        trace.push_back(CommandGFill{ unitZ, (pos[2] - unitZ) - (pos[6] + unitZ)}); // a
        trace.push_back(CommandGFill{ unitZ, (pos[3] - unitZ) - (pos[7] + unitZ)}); // b
        infopoint("face ZY");
    }

    // fill & move
    for (int y = cube.c2.y; y != cube.c1.y + 1; --y) {
        REP(4) trace.push_back(CommandWait{});
        REP(4) trace.push_back(CommandSMove{-unitY});
        REP(4) trace.push_back(CommandWait{});
        REP(4) trace.push_back(CommandFill{unitY});
    }
    infopoint("down by 8");

    // fusion 8 -> 4
    REP(4) trace.push_back(CommandFusionP{unitY});
    REP(4) trace.push_back(CommandFusionS{-unitY});
    infopoint("fusion");

    // fill & move
    for (int y = cube.c1.y; y != 0; --y) {
        REP(4) trace.push_back(CommandSMove{-unitY});
        REP(4) trace.push_back(CommandFill{unitY});
    }
    infopoint("down by 4");

    // to 2x2
    for (int i = 0; i < cube_size - 1; ++i) {
        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandVoid{-unitX}); // 2
        trace.push_back(CommandVoid{-unitX}); // 3
        trace.push_back(CommandVoid{-unitZ}); // 4

        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandWait{}); // 2
        trace.push_back(CommandVoid{-unitZ}); // 3
        trace.push_back(CommandWait{}); // 4

        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandWait{}); // 2
        trace.push_back(CommandVoid{-unitX-unitZ}); // 3
        trace.push_back(CommandWait{}); // 4

        infopoint("2x2 internal");
        auto b0 = state.system.bots[0];
        auto b1 = state.system.bots[1];
        auto b2 = state.system.bots[2];
        auto b3 = state.system.bots[3];
        /*
        LOG() << b0.bid << " " << b1.bid << " " << b2.bid << " " << b3.bid << "\n";
        LOG() << state.system.matrix(b0.pos) << "\n";
        LOG() << state.system.matrix(b1.pos - unitX) << "\n";
        LOG() << state.system.matrix(b2.pos - unitX - unitZ) << "\n";
        LOG() << state.system.matrix(b3.pos - unitZ) << "\n";
        */

        trace.push_back(CommandWait{}); // 1
        trace.push_back(CommandSMove{-unitX}); // 2
        trace.push_back(CommandLMove{-unitX, -unitZ}); // 3
        trace.push_back(CommandSMove{-unitZ}); // 4
        infopoint("2x2 internal");


        if (tgt_matrix(b1.pos + unitX)) {
            trace.push_back(CommandWait{}); // 1
            trace.push_back(CommandFill{+unitX}); // 2
            trace.push_back(CommandWait{}); // 3
            trace.push_back(CommandWait{}); // 4
        }

        if (tgt_matrix(b2.pos + unitX + unitZ)) {
            trace.push_back(CommandWait{});
            trace.push_back(CommandWait{});
            trace.push_back(CommandFill{unitX + unitZ}); // 3
            trace.push_back(CommandWait{});
        }

        if (tgt_matrix(b3.pos + unitZ)) {
            trace.push_back(CommandWait{});
            trace.push_back(CommandWait{});
            trace.push_back(CommandWait{});
            trace.push_back(CommandFill{+ unitZ}); // 4
        }
    }
    infopoint("2x2");

    // fusion 4 -> 2
    trace.push_back(CommandFusionP{+unitZ});
    trace.push_back(CommandFusionP{+unitZ});
    trace.push_back(CommandFusionS{-unitZ});
    trace.push_back(CommandFusionS{-unitZ});
    infopoint("4->2");

    // fusion 2 -> 1
    trace.push_back(CommandFusionP{unitX});
    trace.push_back(CommandFusionS{-unitX});
    infopoint("2->1");

    // home.
    {
        Vec3 p(cube.c1.x, 0, cube.c1.z);
        LOG() << "BOT POS : " << state.system.bots[0].pos << "\n";
        LOG() << "start POS : " << p << "\n";
        NTraceUtil::digging_move(erased_matrix, Vec3(0, 0, 0), p, trace);
    }

    // go home.
    trace.push_back(CommandHalt{});
    infopoint("final");

    return state.system.trace;
}

}  // namespace

REGISTER_ENGINE(final_gfill, solver);
