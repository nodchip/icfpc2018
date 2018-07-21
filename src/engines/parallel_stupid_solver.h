#ifndef __PARALLEL_STUPID_SOLVER_H__
#define __PARALLEL_STUPID_SOLVER_H__

#include "matrix.h"
#include "trace.h"

// parallel stupid solver.
// in general, solvers do not alter the system but only generate a plan for the system.
Trace parallel_stupid_solver(const Matrix& problem_matrix);

#endif // __PARALLEL_STUPID_SOLVER_H__
// vim: set si et sw=4 ts=4:
