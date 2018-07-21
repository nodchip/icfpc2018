// Nanobot Matter Manipulating System
//
#ifndef __NMMS_H__
#define __NMMS_H__

#include "system.h"

// @return well-formed state and matters are identical to problem_matrix.
//         i.e. ready to submit.
bool is_finished(const System& system, const Matrix& problem_matrix);

void global_energy_update(System& system);

// @return true if halted.
bool proceed_timestep(System& system);

// @return true if halted.
bool simulate_all(System& system);

// 6neighbors
bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt);

#endif // __NMMS_H__
// vim: set si et sw=4 ts=4:
