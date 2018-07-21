#pragma once

#include "matrix.h"
#include "system.h"
#include "trace.h"

struct State {
  State(const Matrix& m);

  int simulate(const Trace& t);

  Matrix problem;
  System sys;
};
