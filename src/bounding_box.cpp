#include "bounding_box.h"

Region find_bounding_box(const Matrix& m, std::vector<int>* per_y_voxels_optional) {
    int mins[3] = {m.R, m.R, m.R};
    int maxs[3] = {0, 0, 0};
    if (per_y_voxels_optional) {
        per_y_voxels_optional->clear();
    }
    for (int y = 0; y < m.R; ++y) {
        int y_count = 0;
        for (int z = 0; z < m.R; ++z) {
            for (int x = 0; x < m.R; ++x) {
                if (m(x, y, z)) {
                    ++y_count;
                    mins[0] = std::min(mins[0], x);
                    mins[1] = std::min(mins[1], y);
                    mins[2] = std::min(mins[2], z);
                    maxs[0] = std::max(maxs[0], x);
                    maxs[1] = std::max(maxs[1], y);
                    maxs[2] = std::max(maxs[2], z);
                }
            }
        }
        if (per_y_voxels_optional) {
            per_y_voxels_optional->push_back(y_count);
        }
    }
    return Region(Vec3(mins[0], mins[1], mins[2]), Vec3(maxs[0], maxs[1], maxs[2]));
}

// vim: set si et sw=4 ts=4:
