#pragma once

#include "matrix.h"
#include "system.h"
#include "trace.h"

struct State {
  State(const Matrix& src_matrix, const Matrix& tgt_matrix);

  int simulate(const Trace& t);

  // @return well-formed state and matters are identical to problem_matrix.
  //         i.e. ready to submit.
  bool is_finished() const;

  Matrix src_problem;
  Matrix tgt_problem;
  System system;
};


enum class ProblemType { Assembly, Disassembly, Reassembly, Invalid };
ProblemType determine_problem_type_and_prepare_matrices(Matrix& src_matrix, Matrix& tgt_matrix);

