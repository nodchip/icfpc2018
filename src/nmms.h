// Nanobot Matter Manipulating System
//
#ifndef __NMMS_H__
#define __NMMS_H__

#include "system.h"

// 6neighbors
bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt);

// fast motion for single nanobot.
// generate a (fast) SMove sequence avoiding Full voxels.
bool fast_manhattan_motion_in_void(const Matrix& matrix, Vec3 start_pos, Vec3 stop_pos,
    Trace& trace);

// fast move
// if destination == position. no moves are generated.
void fast_move(const Vec3& destination, Vec3& position, Trace& trace);

namespace NEditPoints {
    // inclusive
    std::vector<Vec3> fill_zigzag_ii(const Vec3& start, const Vec3& diagonal);
    // inclusive
    std::vector<Vec3> move_naive_ii(const Vec3& start, const Vec3& stop);
    bool dedup(std::vector<Vec3>& points);
    bool is_connected_6(const std::vector<Vec3>& points);
}

#endif // __NMMS_H__
// vim: set si et sw=4 ts=4:
