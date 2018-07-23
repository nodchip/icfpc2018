#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include "log.h"

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

    Vec3 operator+() const {
        return *this;
    }
    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }

    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }
    Vec3 operator*(int rhs) const {
        return Vec3(x * rhs, y * rhs, z * rhs);
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
    Vec3& operator*=(int rhs) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }
    Vec3 transpose() {
        return Vec3(z, y, x);
    }

    uint32_t index() const {
        return (uint32_t(x) << 16) | (uint32_t(y) << 8) | uint32_t(z);
    }
    static uint32_t index_end() {
        return 1 << 25;
    }

    void print() const;

    struct hash {
        size_t operator()(const Vec3& s) const {
            // since x, y, z are all 8 bit, just concatenate them.
            return (uint32_t(s.x) << 16) | (uint32_t(s.y) << 8) | uint32_t(s.z);
        }
    };

    bool is_parallel_to_axis() const {
        return (x != 0 && y == 0 && z == 0) ||
            (x == 0 && y != 0 && z == 0) ||
            (x == 0 && y == 0 && z != 0);
    }

    int manhattan_length() const {
        return std::abs(x) + std::abs(y) + std::abs(z);
    }

    Vec3 unit_vector() const {
        ASSERT(is_parallel_to_axis());
        int ml = manhattan_length();
        return Vec3(x / ml, y / ml, z / ml);
    }
};

namespace std {
    template <> struct hash<Vec3> {
        typedef Vec3 argument_type;
        typedef size_t result_type;
        result_type operator()(const argument_type& r) const noexcept {
            return (uint32_t(r.x) << 16) | (uint32_t(r.y) << 8) | uint32_t(r.z);
        }
    };
}

inline std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    return std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline void Vec3::print() const {
    std::cout << *this << std::endl;
}

inline Vec3 abs(Vec3 p) {
    return Vec3(std::abs(p.x), std::abs(p.y), std::abs(p.z));
}
inline Vec3 sign(Vec3 p) {
    return Vec3(
        p.x == 0 ? 0 : (p.x > 0 ? 1 : -1),
        p.y == 0 ? 0 : (p.y > 0 ? 1 : -1),
        p.z == 0 ? 0 : (p.z > 0 ? 1 : -1));
}
inline int length2(Vec3 p) {
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

inline int largest_abs_axis(Vec3 p) {
    auto a = abs(p);
    if (a.y > a.x && a.y > a.z) return 1;
    if (a.z > a.x && a.z > a.y) return 2;
    return 0;
}

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

extern Vec3 neighbors26[26];
extern Vec3 neighbors18[18];
extern Vec3 neighbors6[6];
// vim: set si et sw=4 ts=4:
