#include "vec3.h"

std::vector<Vec3> neighbors26() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x != 0 || y != 0 || z != 0) {
                    neighbors.emplace_back(x, y, z);
                }
            }
        }
    }
    return neighbors;
}

std::vector<Vec3> neighbors18() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x != 0 || y != 0 || z != 0) {
                    if (x == 0 || y == 0 || z == 0) {
                        neighbors.emplace_back(x, y, z);
                    }
                }
            }
        }
    }
    return neighbors;
}

std::vector<Vec3> neighbors6() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if ((x != 0) + (y != 0) + (z != 0) == 1) {
                    neighbors.emplace_back(x, y, z);
                }
            }
        }
    }
    return neighbors;
}
