#include "stupid_solver.h"

#include "engine.h"
#include "nmms.h"
#include "system.h"
#include "state.h"
#include "debug_message.h"


Trace stupid_solver_disassembly(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    ASSERT_ERROR_RETURN(problem_type == ProblemType::Disassembly, Trace());

    printf("stupid_solver\n");
    System system(src_matrix.R);
    Trace trace;

    // go to the top plane. there are no blocker around the path.
    const int R = src_matrix.R;
    Vec3 p = system.bots[0].pos;
    fast_move(Vec3(0, R - 1, 0), p, trace);

    // erase "bottom" voxels in zig-zag manner.
    trace.push_back(CommandFlip{}); // high.
    auto try_void_bottom = [&] {
        Vec3 bottom = p + Vec3(0, -1, 0);
        if (src_matrix.is_in_matrix(bottom) && src_matrix(bottom)) {
            trace.push_back(CommandVoid{bottom - p});
        }
    };
    while (true) {
        int zdir = p.y % 2 == 0 ? +1 : -1;
        while (0 <= p.z + zdir && p.z + zdir < R) {
            int xdir = p.z % 2 == 0 ? +1 : -1;
            while (0 <= p.x + xdir && p.x + xdir < R) {
                p.x += xdir;
                trace.push_back(CommandSMove{Vec3(xdir, 0, 0)});
                try_void_bottom();
            }
            // turn around.
            p.z += zdir;
            trace.push_back(CommandSMove{Vec3(0, 0, zdir)});
            try_void_bottom();
        }
        if (0 <= p.y - 1) {
            // down.
            p.y -= 1;
            trace.push_back(CommandSMove{Vec3(0, -1, 0)});
            try_void_bottom();
        } else {
            break;
        }
    }
    trace.push_back(CommandFlip{}); // low.

    // go home.
    fast_move(system.final_pos(), p, trace);

    // finalize at the origin pos.
    trace.push_back(CommandHalt{});
    return trace;
}

Trace stupid_solver_assembly(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    ASSERT_ERROR_RETURN(problem_type == ProblemType::Assembly, Trace());
    
    printf("stupid_solver\n");
    System system(dst_matrix.R);

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
                if (dst_matrix(prev)) {
                    trace.push_back(CommandFill{prev - p});
                }
                prev = p;
            }
            // turn around.
            p.z += zdir;
            trace.push_back(CommandSMove{Vec3(0, 0, zdir)});
            if (dst_matrix(prev)) {
                trace.push_back(CommandFill{prev - p});
            }
            prev = p;
        }
        if (p.y + 1 < system.matrix.R) {
            // up.
            p.y += 1;
            trace.push_back(CommandSMove{Vec3(0, 1, 0)});
            if (dst_matrix(prev)) {
                trace.push_back(CommandFill{prev - p});
            }
            prev = p;
        } else {
            break;
        }
    }

    // go home.
    std::vector<Vec3> trajectory;
    if (!bfs_shortest_in_void(dst_matrix, p, system.final_pos(),
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

Trace stupid_solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    switch (problem_type) {
        case ProblemType::Assembly:
            return stupid_solver_assembly(problem_type, src_matrix, dst_matrix);
        case ProblemType::Disassembly:
            return stupid_solver_disassembly(problem_type, src_matrix, dst_matrix);
        case ProblemType::Reassembly:
        default:
            break;
            //return stupid_solver_reassembly(problem_type, src_matrix, dst_matrix);
    }
    ASSERT_ERROR_RETURN(false, Trace());
}

REGISTER_ENGINE(stupid, stupid_solver);
// vim: set si et sw=4 ts=4:
