#pragma once

#include "matrix.h"
#include "system.h"
#include "trace.h"

struct State {
  State(const Matrix& m);

  int simulate(const Trace& t);

  // @return well-formed state and matters are identical to problem_matrix.
  //         i.e. ready to submit.
  bool is_finished() const;

  Matrix problem;
  System system;
};
