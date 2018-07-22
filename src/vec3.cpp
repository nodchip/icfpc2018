#include "vec3.h"

Vec3 neighbors26[26];
Vec3 neighbors18[18];
Vec3 neighbors6[6];

struct NeighborInitializer {
    NeighborInitializer() {
        {
            int index = 0;
            for (int z = -1; z <= 1; ++z) {
                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        if (x != 0 || y != 0 || z != 0) {
                            neighbors26[index++] = Vec3(x, y, z);
                        }
                    }
                }
            }
        }
        {
            int index = 0;
            for (int z = -1; z <= 1; ++z) {
                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        if (x != 0 || y != 0 || z != 0) {
                            if (x == 0 || y == 0 || z == 0) {
                                neighbors18[index++] = Vec3(x, y, z);
                            }
                        }
                    }
                }
            }
        }
        {
            int index = 0;
            for (int z = -1; z <= 1; ++z) {
                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        if ((x != 0) + (y != 0) + (z != 0) == 1) {
                            neighbors6[index++] = Vec3(x, y, z);
                        }
                    }
                }
            }
        }
    }
} neighbor_initializer;
