#pragma once

#include "matrix.h"
#include "trace.h"
#include "state.h"

Trace cubee(ProblemType problem_type, const Matrix& src_matrix, const Matrix& problem_matrix);
