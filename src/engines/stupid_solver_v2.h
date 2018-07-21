#ifndef __STUPID_SOLVER_V2_H__
#define __STUPID_SOLVER_V2_H__

#include "matrix.h"
#include "trace.h"

// super stupid solver.
// in general, solvers do not alter the system but only generate a plan for the system.
Trace stupid_solver_v2(const Matrix& problem_matrix);

#endif // __STUPID_SOLVER_V2_H__
// vim: set si et sw=4 ts=4:
