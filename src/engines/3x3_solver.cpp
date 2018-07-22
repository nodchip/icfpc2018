#include "3x3_solver.h"
#include <boost/range/algorithm_ext/push_back.hpp>

#include "engine.h"
#include "log.h"
#include "nmms.h"
#include "state.h"
#include "system.h"

static const Vec3 kPlaneNeighbor8[] = {
    Vec3(-1, -1, 0),
    Vec3(-1, 0, 0),
    Vec3(-1, 1, 0),
    Vec3(0, -1, 0),
    Vec3(0, 1, 0),
    Vec3(1, -1, 0),
    Vec3(1, 0, 0),
    Vec3(1, 1, 0),
};

void DoFill(const Vec3& current_position, const Vec3& full_direction, Matrix& current_matrix, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + full_direction));
    ASSERT(current_matrix(current_position + full_direction) == Void);
    current_matrix(current_position + full_direction) = Full;
    trace.push_back(CommandFill{ full_direction });
}

void DoVoid(const Vec3& current_position, const Vec3& void_direction, Matrix& current_matrix, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + void_direction));
    ASSERT(current_matrix(current_position + void_direction) == Full);
    current_matrix(current_position + void_direction) = Void;
    trace.push_back(CommandVoid{ void_direction });
}

void DoSMove(const Matrix& current_matrix, const Vec3& move_direction, Vec3& current_position, Trace& trace) {
    ASSERT(current_matrix.is_in_matrix(current_position + move_direction));
    current_position += move_direction;
    trace.push_back(CommandSMove{ move_direction });
}

void dig_to_x(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position, int x_direction) {
    int R = dst_matrix.R;
    Vec3 front_direction(x_direction, 0, 0);
    Vec3 back_direction(-x_direction, 0, 0);
    for (int i = 0; i < 3; ++i) {
        // Void if the front voxel if full.
        if (current_matrix(current_position + front_direction) == Full) {
            DoVoid(current_position, front_direction, current_matrix, trace);
        }

        // Move to the front voxel.
        DoSMove(current_matrix, front_direction, current_position, trace);

        // Full if the back voxel as needed.
        ASSERT(current_matrix(current_position + back_direction) == Void);
        if (dst_matrix(current_position + back_direction) == Full) {
            DoFill(current_position, back_direction, current_matrix, trace);
        }
    }
}

void dig_to_y(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position) {
    int R = dst_matrix.R;
    Vec3 front_direction(0, 1, 0);
    Vec3 back_direction(0, -1, 0);
    for (int i = 0; i < 3; ++i) {
        // Void if the front voxel if full.
        if (current_matrix(current_position + front_direction) == Full) {
            DoVoid(current_position, front_direction, current_matrix, trace);
        }

        // Move to the front voxel.
        DoSMove(current_matrix, front_direction, current_position, trace);

        // Full if the back voxel as needed.
        ASSERT(current_matrix(current_position + back_direction) == Void);
        if (dst_matrix(current_position + back_direction) == Full) {
            DoFill(current_position, back_direction, current_matrix, trace);
        }
    }
}

void dig_to_z(const Matrix& dst_matrix, Matrix& current_matrix, Trace& trace, Vec3& current_position, int z_direction) {
    int R = dst_matrix.R;
    Vec3 front_direction(0, 0, z_direction);
    Vec3 back_direction(0, 0, -z_direction);
    for (;;) {
        // Full or void x-y 8-neighbor voxels.
        for (const auto& direction : kPlaneNeighbor8) {
            if (!dst_matrix.is_in_matrix(current_position + direction)) {
                continue;
            }

            if (current_matrix(current_position + direction) == Full && dst_matrix(current_position + direction) == Void) {
                DoVoid(current_position, direction, current_matrix, trace);
            }
            else if (current_matrix(current_position + direction) == Void && dst_matrix(current_position + direction) == Full) {
                DoFill(current_position, direction, current_matrix, trace);
            }
        }

        // Full the back voxel as needed.
        ASSERT(current_matrix(current_position + back_direction) == Void);
        if (dst_matrix(current_position + back_direction) == Full) {
            DoFill(current_position, back_direction, current_matrix, trace);
        }

        if (current_position.z + z_direction < 1 || R - 2 < current_position.z + z_direction) {
            break;
        }

        // Void the front voxel as needed.
        if (current_matrix(current_position + front_direction) == Full) {
            DoVoid(current_position, front_direction, current_matrix, trace);
        }

        // Move to the front voxel.
        ASSERT(current_matrix(current_position + front_direction) == Void);
        DoSMove(current_matrix, front_direction, current_position, trace);
    }
}

Trace three_by_three_solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& dst_matrix) {
    printf("three_by_three_solver\n");
    const int R = src_matrix.R;
    System system(R);
    Trace trace;

    Matrix current_matrix = src_matrix;
    const Vec3 start_position(0, 0, 0);
    const Vec3 zigzag_ready_position(2, 1, 0);
    const Vec3 zigzag_start_position(2, 1, 1);
    Vec3 current_position(0, 0, 0);

    // high.
    trace.push_back(CommandFlip{}); 

    // Move to the start position.
    std::vector<Vec3> trajectory;
    if (!bfs_shortest_in_void(current_matrix, current_position, zigzag_ready_position, &trace, &trajectory)) {
        std::cout << "Failed to move the zigzag ready position..." << std::endl;
        return trace;
    }
    current_position = zigzag_ready_position;

    // Void the zigzag start position if it is full.
    if (current_matrix(zigzag_start_position)) {
        DoVoid(current_position, { 0, 0, 1 }, current_matrix, trace);
    }

    // Move to the zigzag start position.
    DoSMove(current_matrix, { 0, 0, 1 }, current_position, trace);

    // ZigZag
    int x_direction = 1;
    int z_direction = 1;
    for (;;) {
        dig_to_z(dst_matrix, current_matrix, trace, current_position, z_direction);
        z_direction = -z_direction;
        if (dst_matrix.is_in_matrix(current_position + Vec3(x_direction, 0, 0) * 3)) {
            dig_to_x(dst_matrix, current_matrix, trace, current_position, x_direction);
        }
        else if (dst_matrix.is_in_matrix(current_position + Vec3(0, 3, 0))) {
            x_direction = -x_direction;
            dig_to_y(dst_matrix, current_matrix, trace, current_position);
        }
        else {
            break;
        }
    }

    if (dst_matrix(current_position) == Full) {
        // Move +z by one voxel to fill the last voxel.
        DoSMove(current_matrix, { 0, 0, 1 }, current_position, trace);
        DoFill(current_position, { 0, 0, -1 }, current_matrix, trace);
    }

    // Move to the start position.
    if (!bfs_shortest_in_void(current_matrix, current_position, start_position, &trace, &trajectory)) {
        std::cout << "Failed to move to the start position..." << std::endl;
        return trace;
    }

    trace.push_back(CommandFlip{}); // low.
    trace.push_back(CommandHalt{});

    return trace;
}

REGISTER_ENGINE(stupid, three_by_three_solver);
// vim: set si et sw=4 ts=4:
