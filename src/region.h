#pragma once

#include <vector>
#include <boost/functional/hash.hpp>

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

    bool is_in_region(Vec3 p) const {
        auto r = canonical();
        return r.c1.x <= p.x && p.x <= r.c2.x
            && r.c1.y <= p.y && p.y <= r.c2.y
            && r.c1.z <= p.z && p.z <= r.c2.z;
    }

};

namespace std {
    template <> struct hash<Region> {
        typedef Region argument_type;
        typedef size_t result_type;
        result_type operator()(const argument_type& r) const noexcept {
            result_type hash = Vec3::hash()(r.c1);
            boost::hash_combine(hash, Vec3::hash()(r.c2));
            return hash;
        }
    };
}

inline Region core_region(int R) {
    return Region(Vec3(1, 1, 1), Vec3(R-2, R-2, R-2));
}

#define CANONICAL_REGION_FOR(r, xx, yy, zz) \
    for (int yy = (r).c1.y; yy <= (r).c2.y; ++yy) \
        for (int zz = (r).c1.z; zz <= (r).c2.z; ++zz) \
            for (int xx = (r).c1.x; xx <= (r).c2.x; ++xx) 
    
