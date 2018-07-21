#pragma once

#include "matrix.h"

Region find_bounding_box(const Matrix& m, std::vector<int>* per_y_voxels_optional);
// vim: set si et sw=4 ts=4:
