#ifndef __NAIVE_CONVERTER_H__
#define __NAIVE_CONVERTER_H__

#include <functional>

#include "matrix.h"
#include "trace.h"
#include "state.h"

namespace NaiveConverter {

using SolverType = std::function<Trace(ProblemType, const Matrix&, const Matrix&)>;

// Assembly solver -> Disassembly solver
SolverType reverse(const SolverType& assembly_solver);

// Disassembly and Assembly solvers -> Reassembly solver
SolverType concatenate(const SolverType& disassembly_solver, const SolverType& assembly_solver);

}  // namespace NaiveConverter

#endif // __NAIVE_CONVERTER_H__
// vim: set si et sw=4 ts=4:
