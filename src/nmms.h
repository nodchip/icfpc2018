// Nanobot Matter Manipulating System
//

#include <vector>
#include <boost/variant.hpp>

struct Vec3 {
    Vec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    int x = 0, y = 0, z = 0;
    bool operator==(const Vec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    int& operator[](size_t i) {
        int* p[] = {&x, &y, &z};
        return *p[i];
    }
    int operator[](size_t i) const {
        int p[] = {x, y, z};
        return p[i];
    }
};

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

typedef std::vector<Command> Trace;
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

    int R;
    std::vector<uint8_t> buf;
};

Matrix load_model(std::string input_path);
bool dump_model(std::string output_path, const Matrix& m);

// vim: set si et sw=4 ts=4:
