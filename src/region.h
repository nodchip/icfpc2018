#pragma once

#include <vector>

#include "vec3.h"

struct Region {
    Region(Vec3 c1_, Vec3 c2_) : c1(c1_), c2(c2_) {}
    Vec3 c1;
    Vec3 c2;
    bool operator==(const Region& rhs) const {
        Region r = rhs.canonical();
        return c1 == r.c1 && c2 == r.c2;
    }
    Region canonical() const {
        return Region {
            Vec3(std::min(c1.x, c2.x), std::min(c1.y, c2.y), std::min(c1.z, c2.z)),
            Vec3(std::max(c1.x, c2.x), std::max(c1.y, c2.y), std::max(c1.z, c2.z))
            };
    }
};
