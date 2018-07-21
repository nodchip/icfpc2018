#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "region.h"
#include "vec3.h"

enum Voxel : uint8_t {
  Void = 0,
  Full = 1,
};

struct Matrix {
    Matrix(int R_) : R(R_), buf(R*R*R, Void) {
    }

    Voxel& operator()(int x, int y, int z) {
        return buf[(z * R + y) * R + x];
    }
    Voxel operator()(int x, int y, int z) const {
        return buf[(z * R + y) * R + x];
    }
    Voxel& operator()(const Vec3& p) { return operator()(p.x, p.y, p.z); }
    Voxel operator()(const Vec3& p) const { return operator()(p.x, p.y, p.z); }

    // is this a valid matrix?
    operator bool() const {
        return 0 < R && !buf.empty();
    }

    bool is_in_matrix(int x, int y, int z) const {
         return (0 <= x && x < R) && (0 <= y && y < R) && (0 <= z && z < R);
    }
    bool is_in_matrix(const Vec3& p) const { return is_in_matrix(p.x, p.y, p.z); }

    bool any_full(Region r) const {
        r = r.canonical();
        for (int z = r.c1.z; z <= r.c2.z; ++z) {
            for (int y = r.c1.y; y <= r.c2.y; ++y) {
                for (int x = r.c1.x; x <= r.c2.x; ++x) {
                    if ((*this)(x, y, z)) return true;
                }
            }
        }
        return false;
    }

    int R;
    std::vector<Voxel> buf;
};
typedef std::shared_ptr<Matrix> MatrixPtr;
