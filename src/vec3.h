#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

struct Vec3 {
    Vec3() : x(0), y(0), z(0) {}
    Vec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    int x = 0, y = 0, z = 0;

    bool operator==(const Vec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    bool operator!=(const Vec3& rhs) const {
        return !((*this) == rhs);
    }
    int& operator[](size_t i) {
        int* p[] = {&x, &y, &z};
        return *p[i];
    }
    int operator[](size_t i) const {
        int p[] = {x, y, z};
        return p[i];
    }

    bool operator<(const Vec3& rhs) const {
        if (x != rhs.x) return x < rhs.x;
        if (y != rhs.y) return y < rhs.y;
        return z < rhs.z;
    }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vec3& operator+=(const Vec3& rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    Vec3& operator-=(const Vec3& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    void print() const {
        std::printf("(%d, %d, %d)\n", x, y, z);
    }

    struct hash {
        size_t operator()(const Vec3& s) const {
            // since x, y, z are all 8 bit, just concatenate them.
            return (uint32_t(s.x) << 16) | (uint32_t(s.y) << 8) | uint32_t(s.z);
        }
    };
};

// Manhattan distance
inline int mlen(Vec3 v) {
    return std::abs(v.x) + std::abs(v.y) + std::abs(v.z);
}
// checkerboard distance
inline int clen(Vec3 v) {
    return std::max(std::max(std::abs(v.x), std::abs(v.y)),  std::abs(v.z));
}

inline Vec3 linear_coordinate_difference_x(int x) { return Vec3(x, 0, 0); }
inline Vec3 linear_coordinate_difference_y(int y) { return Vec3(0, y, 0); }
inline Vec3 linear_coordinate_difference_z(int z) { return Vec3(0, 0, z); }

std::vector<Vec3> neighbors26();
std::vector<Vec3> neighbors18();
std::vector<Vec3> neighbors6();
