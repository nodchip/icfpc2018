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
    CommandFusionP,
    CommandFusionS> Command;
