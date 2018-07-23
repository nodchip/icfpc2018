#ifndef __PARALLEL_STUPID_SOLVER_V3_1_H__
#define __PARALLEL_STUPID_SOLVER_V3_1_H__

#include "matrix.h"
#include "trace.h"
#include "state.h"

// parallel stupid solver v3.
// in general, solvers do not alter the system but only generate a plan for the system.
Trace parallel_stupid_solver_v3_1(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix);

#endif // __PARALLEL_SOLVER_V2_H__
// vim: set si et sw=4 ts=4:
