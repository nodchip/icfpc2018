// Nanobot Matter Manipulating System
//
#ifndef __NMMS_H__
#define __NMMS_H__

#include "system.h"

// 6neighbors
bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt);

#endif // __NMMS_H__
// vim: set si et sw=4 ts=4:
