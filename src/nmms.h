// Nanobot Matter Manipulating System
//
#ifndef __NMMS_H__
#define __NMMS_H__

#include <vector>
#include <deque>
#include <memory>
#include <boost/variant.hpp>

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
        printf("(%d, %d, %d)\n", x, y, z);
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

struct Region {
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


struct CommandHalt {};
struct CommandWait {};
struct CommandFlip {};
struct CommandSMove {
    Vec3 lld;
};
struct CommandLMove {
    Vec3 sld1;
    Vec3 sld2;
};
struct CommandFission {
    Vec3 nd;
    int m;
};
struct CommandFill {
    Vec3 nd;
};
struct CommandFusionP {
    Vec3 nd;
};
struct CommandFusionS {
    Vec3 nd;
};

namespace Costs {
    constexpr int k_SMove = 2;
    constexpr int k_LMove = 2;
    constexpr int k_LMoveOffset = 2;
    constexpr int k_FillVoid = 12;
    constexpr int k_FillFull = 6;
}

typedef boost::variant<
    CommandHalt,
    CommandWait,
    CommandFlip,
    CommandSMove,
    CommandLMove,
    CommandFission,
    CommandFill,
    CommandFusionP,
    CommandFusionS> Command;

typedef std::deque<Command> Trace;
bool output_trace(std::string output_path, const Trace& trace);


enum { Void = 0, Full = 1 };
struct Matrix {
    Matrix(int R_) : R(R_), buf(R*R*R, Void) {
    }

    uint8_t& operator()(int x, int y, int z) {
        return buf[(z * R + y) * R + x];
    }
    uint8_t operator()(int x, int y, int z) const {
        return buf[(z * R + y) * R + x];
    }
    uint8_t& operator()(const Vec3& p) { return operator()(p.x, p.y, p.z); }
    uint8_t operator()(const Vec3& p) const { return operator()(p.x, p.y, p.z); }

    // is this a valid matrix?
    operator bool() const {
        return 0 < R && !buf.empty();
    }

    bool is_in_matrix(int x, int y, int z) const {
         return (0 <= x && x < R) && (0 <= y && y < R) && (0 <= z && z < R);
    }
    bool is_in_matrix(const Vec3& p) const { return is_in_matrix(p.x, p.y, p.z); }

    int R;
    std::vector<uint8_t> buf;
};
typedef std::shared_ptr<Matrix> MatrixPtr;

Matrix load_model(std::string input_path);
bool dump_model(std::string output_path, const Matrix& m);


typedef uint32_t BotID;
struct Bot {
    BotID bid = 1;
    Vec3 pos = {0, 0, 0};
    std::vector<BotID> seeds;
};
struct System {
    int64_t energy = 0;
    bool harmonics_high = false;
    Matrix matrix = {0};
    std::vector<Bot> bots;
    Trace trace;

    int64_t consumed_commands = 0; // not necessary for the game.

    static Vec3 start_pos() { return Vec3(0, 0, 0); }
    static Vec3 final_pos() { return Vec3(0, 0, 0); }

    bool Start(int R);

    void print() {
        std::printf("System energy=%ld, harmonics=%s, bots=%ld, trace=%ld commands=%ld\n",
            energy, harmonics_high ? "high" : "low",
            bots.size(), trace.size(), consumed_commands);
    }
};

// @return well-formed state and matters are identical to problem_matrix.
//         i.e. ready to submit.
bool is_finished(const System& system, const Matrix& problem_matrix);

// @return true if halted.
bool proceed_timestep(System& system);

// @return true if halted.
bool simulate_all(System& system);

// 6neighbors
bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt);

#endif // __NMMS_H__
// vim: set si et sw=4 ts=4:
