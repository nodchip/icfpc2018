#include "engine.h"
#include "region.h"
#include "bounding_box.h"
#include "nmms.h"

Trace stupid_bbox_solver(const System& system, const Matrix& problem_matrix) {
    // use a single nanobot.
    // always in the high harmonics.
    // zig-zag scanning in the XZ plane.
    // only fill the "previous" voxels.
    // go back to the home using BFS. (it is possible to lock out itself)

    Trace trace;
    trace.push_back(CommandFlip{}); // high.

    Vec3 p(system.bots[0].pos);

    // move to bbox.
    Region bbox = find_bounding_box(problem_matrix, nullptr).canonical();
    bfs_shortest_in_void(system.matrix, p, bbox.c1, &trace, nullptr);
    p = bbox.c1;
    
    Vec3 prev = p;
    while (true) {
        int zdir = p.y % 2 == 0 ? +1 : -1;
        while (bbox.c1.z <= p.z + zdir && p.z + zdir <= bbox.c2.z) {
            int xdir = p.z % 2 == 0 ? +1 : -1;
            while (bbox.c1.x <= p.x + xdir && p.x + xdir <= bbox.c2.x) {
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
        if (p.y + 1 <= bbox.c2.y) {
            // up.
            p.y += 1;
            trace.push_back(CommandSMove{Vec3(0, 1, 0)});
        } else {
            break;
        }
    }

    // go home.
    if (!bfs_shortest_in_void(system.matrix, p, system.final_pos(),
        &trace, nullptr)) {
        std::cout << "sorry, stupid algorithm failed.." << std::endl;
        return trace;
    }

    // finalize at the origin pos.
    trace.push_back(CommandFlip{});
    trace.push_back(CommandHalt{});
    return trace;
}

REGISTER_ENGINE(stupid_bbox, stupid_bbox_solver);
// vim: set si et sw=4 ts=4:
