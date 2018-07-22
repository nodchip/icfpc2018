#include "stupid_solver.h"
#include <boost/range/algorithm_ext/push_back.hpp>

#include "engine.h"
#include "log.h"
#include "nmms.h"
#include "state.h"
#include "system.h"

Trace stupid_solver_reassembly(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    ASSERT_RETURN(problem_type == ProblemType::Reassembly, Trace());

    printf("stupid_solver\n");
    const int R = src_matrix.R;
    System system(R);
    Trace trace;

    const Vec3 start(0, 0, 0);
    const Vec3 top(0, R - 1, 0);
    const Vec3 diagonal(R - 1, 0, R - 1);

    std::vector<Vec3> scan = NEditPoints::move_naive_ii(start, top);
    boost::push_back(scan, NEditPoints::fill_zigzag_ii(top, diagonal));
    boost::push_back(scan, NEditPoints::move_naive_ii(scan.back(), start));
    NEditPoints::dedup(scan);
    ASSERT(NEditPoints::is_connected_6(scan));

    trace.push_back(CommandFlip{}); // high.
    for (size_t i = 0; i < scan.size(); ++i) {
        if (i + 1 < scan.size()) {
            if (src_matrix(scan[i + 1])) {
                trace.push_back(CommandVoid{scan[i + 1] - scan[i]});
            }
            trace.push_back(CommandSMove{scan[i + 1] - scan[i]});
        }
        // now we are at i+1.
        if (dst_matrix(scan[i])) {
            trace.push_back(CommandFill{scan[i] - scan[i + 1]});
        }
    }
    trace.push_back(CommandFlip{}); // low.
    trace.push_back(CommandHalt{});
    LOG() << "trace " << trace.size() << ", scan " << scan.size() << "\n";

    return trace;
}

Trace stupid_solver_disassembly(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    ASSERT_RETURN(problem_type == ProblemType::Disassembly, Trace());

    LOG() << "stupid_solver\n";
    const int R = src_matrix.R;
    System system(R);
    Trace trace;

    const Vec3 start(0, 0, 0);
    const Vec3 top(0, R - 1, 0);
    const Vec3 diagonal(R - 1, 0, R - 1);
    const Vec3 down(0, -1, 0);
    std::vector<Vec3> scan = NEditPoints::move_naive_ii(start, top);
    boost::push_back(scan, NEditPoints::fill_zigzag_ii(top, diagonal));
    boost::push_back(scan, NEditPoints::move_naive_ii(scan.back(), start));
    NEditPoints::dedup(scan);
    ASSERT(NEditPoints::is_connected_6(scan));

    trace.push_back(CommandFlip{}); // high.
    for (size_t i = 0; i < scan.size(); ++i) {
        Vec3 bottom = scan[i] + down;
        if (src_matrix.is_in_matrix(bottom) && src_matrix(bottom)) {
            trace.push_back(CommandVoid{down});
        }
        if (i + 1 < scan.size()) {
            trace.push_back(CommandSMove{scan[i + 1] - scan[i]});
        }
    }
    trace.push_back(CommandFlip{}); // low.
    trace.push_back(CommandHalt{});
    LOG() << "trace " << trace.size() << ", scan " << scan.size() << "\n";

    return trace;
}

Trace stupid_solver_assembly(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    ASSERT_RETURN(problem_type == ProblemType::Assembly, Trace());

    LOG() << "stupid_solver\n";
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
            return stupid_solver_reassembly(problem_type, src_matrix, dst_matrix);
    }
    ASSERT_RETURN(false, Trace());
}

REGISTER_ENGINE(stupid, stupid_solver);
// vim: set si et sw=4 ts=4:
