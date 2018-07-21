#pragma once

#include <boost/variant.hpp>
#include "vec3.h"

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
struct CommandVoid {
    Vec3 nd;
};
struct CommandGFill {
    Vec3 nd;
    Vec3 fd;
};
struct CommandGVoid {
    Vec3 nd;
    Vec3 fd;
};
struct CommandFusionP {
    Vec3 nd;
};
struct CommandFusionS {
    Vec3 nd;
};

namespace Costs {
    // global
    constexpr int k_HighHarmonics = 30;
    constexpr int k_LowHarmonics = 3;
    constexpr int k_Bot = 20;

    // command
    constexpr int k_SMove = 2;
    constexpr int k_LMove = 2;
    constexpr int k_LMoveOffset = 2;
    constexpr int k_FillVoid = 12;
    constexpr int k_FillFull = 6;
    constexpr int k_VoidVoid = 3;
    constexpr int k_VoidFull = -12;
    constexpr int k_GFillVoid = 12;
    constexpr int k_GFillFull = 6;
    constexpr int k_GVoidVoid = 3;
    constexpr int k_GVoidFull = -12;
    constexpr int k_Fission = 24;
    constexpr int k_Fusion = -24;
}

typedef boost::variant<
    CommandHalt,
    CommandWait,
    CommandFlip,
    CommandSMove,
    CommandLMove,
    CommandFission,
    CommandFill,
    CommandVoid,
    CommandGFill,
    CommandGVoid,
    CommandFusionP,
    CommandFusionS> Command;

// ld: Linear Coordinate Difference
inline bool is_valid_ld(Vec3 d) {
    return ((d[0] == 0) + (d[1] == 0) + (d[2] == 0) == 2);
} 
// lld: Long Linear Coordinate Difference
inline bool is_valid_long_ld(Vec3 d) {
    return ((d[0] == 0) + (d[1] == 0) + (d[2] == 0) == 2) && mlen(d) <= 15;
} 
// sld: Short Linear Coordinate Difference
inline bool is_valid_short_ld(Vec3 d) {
    return ((d[0] == 0) + (d[1] == 0) + (d[2] == 0) == 2) && mlen(d) <= 5;
} 
// nd: Near Coordinate Differences
inline bool is_valid_nd(Vec3 d) {
    int m = mlen(d);
    return 0 < m && m <= 2 && clen(d) == 1;
} 
// nd: Far Coordinate Differences
inline bool is_valid_fd(Vec3 d) {
    return 0 < mlen(d) && clen(d) <= 30;

} 

