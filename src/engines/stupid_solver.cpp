#include "engine.h"
#include "stupid_solver.h"

Trace stupid_solver(const System& system, const Matrix& problem_matrix) {
    // use a single nanobot.
    // always in the high harmonics.
    // zig-zag scanning in the XZ plane.
    // only fill the "previous" voxels.
    // go back to the home using BFS. (it is possible to lock out itself)

    Trace trace;
    trace.push_back(CommandFlip{}); // high.

    Vec3 p(system.bots[0].pos);
    Vec3 prev = p;
    while (true) {
        int zdir = p.y % 2 == 0 ? +1 : -1;
        while (0 <= p.z + zdir && p.z + zdir < system.matrix.R) {
            int xdir = p.z % 2 == 0 ? +1 : -1;
            while (0 <= p.x + xdir && p.x + xdir < system.matrix.R) {
                p.x += xdir;
                trace.push_back(CommandSMove{Vec3(xdir, 0, 0)});
                if (problem_matrix(prev)) {
                    trace.push_back(CommandFill{prev - p});
                }
                prev = p;
            }
            // turn around.
            p.z += zdir;
            trace.push_back(CommandSMove{Vec3(0, 0, zdir)});
            if (problem_matrix(prev)) {
                trace.push_back(CommandFill{prev - p});
            }
            prev = p;
        }
        if (p.y + 1 < system.matrix.R) {
            // up.
            p.y += 1;
            trace.push_back(CommandSMove{Vec3(0, 1, 0)});
        } else {
            break;
        }
    }

    // go home.
    std::vector<Vec3> trajectory;
    if (!bfs_shortest_in_void(problem_matrix, p, system.final_pos(),
        &trace, &trajectory)) {
        std::cout << "sorry, stupid algorithm failed.." << std::endl;
        return trace;
    }

    if (false) {
        for (auto p : trajectory) {
            p.print();
        }
    }

    // finalize at the origin pos.
    trace.push_back(CommandFlip{});
    trace.push_back(CommandHalt{});
    return trace;
}

REGISTER_ENGINE(stupid, stupid_solver);
// vim: set si et sw=4 ts=4:
