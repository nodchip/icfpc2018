#include "naive_move.h"

#include "../trace.h"
#include "../vec3.h"

static const Vec3 unitX(1, 0, 0);
static const Vec3 unitY(0, 1, 0);
static const Vec3 unitZ(0, 0, 1);

void naive_move(const Vec3& destination, Vec3& position, Trace& trace) {
    while (position.z != destination.z) {
        auto dist = destination.z - position.z;
        dist = std::max<int>(-15, std::min<int>(dist, 15));
        trace.push_back(CommandSMove{Vec3(0, 0, dist)});
        position += Vec3(0, 0, dist);
    }

    while (position.x != destination.x) {
        auto dist = destination.x - position.x;
        dist = std::max<int>(-15, std::min<int>(dist, 15));
        trace.push_back(CommandSMove{Vec3(dist, 0, 0)});
        position += Vec3(dist, 0, 0);
    }
    while (position.y != destination.y) {
        auto dist = destination.y - position.y;
        dist = std::max<int>(-15, std::min<int>(dist, 15));
        trace.push_back(CommandSMove{Vec3(0, dist, 0)});
        position += Vec3(0, dist, 0);
    }
}
