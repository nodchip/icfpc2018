#include "naive_move.h"

#include "../trace.h"
#include "../vec3.h"

static const Vec3 unitX(1, 0, 0);
static const Vec3 unitY(0, 1, 0);
static const Vec3 unitZ(0, 0, 1);

void naive_move(const Vec3& destination, Vec3& position, Trace& trace, EReduction reduction) {
    if (reduction == ENodchipReduction) {
        Trace temp_trace;
        const Vec3 dz = (destination.z > position.z) ? unitZ : -unitZ;
        while (position.z != destination.z) {
            temp_trace.push_back(CommandSMove{ dz });
            position += dz;
        }

        const Vec3 dx = (destination.x > position.x) ? unitX : -unitX;
        while (position.x != destination.x) {
            temp_trace.push_back(CommandSMove{ dx });
            position += dx;
        }
        const Vec3 dy = (destination.y > position.y) ? unitY : -unitY;
        while (position.y != destination.y) {
            temp_trace.push_back(CommandSMove{ dy });
            position += dy;
        }
        temp_trace.reduction_move();
        trace.insert(trace.end(), temp_trace.begin(), temp_trace.end());
    } else if (reduction == EFofReduction) {
        while (position.z != destination.z) {
            auto dist = destination.z - position.z;
            dist = std::max<int>(-15, std::min<int>(dist, 15));
            trace.push_back(CommandSMove{ Vec3(0, 0, dist) });
            position += Vec3(0, 0, dist);
        }

        while (position.x != destination.x) {
            auto dist = destination.x - position.x;
            dist = std::max<int>(-15, std::min<int>(dist, 15));
            trace.push_back(CommandSMove{ Vec3(dist, 0, 0) });
            position += Vec3(dist, 0, 0);
        }
        while (position.y != destination.y) {
            auto dist = destination.y - position.y;
            dist = std::max<int>(-15, std::min<int>(dist, 15));
            trace.push_back(CommandSMove{ Vec3(0, dist, 0) });
            position += Vec3(0, dist, 0);
        }
    } else if (reduction == ENoReduction) {
        auto dz = position.z < destination.z ? unitZ : -unitZ;
        while (position.z != destination.z) {
            trace.push_back(CommandSMove{dz});
            position += dz;
        }
        auto dx = position.x < destination.x ? unitX : -unitX;
        while (position.x != destination.x) {
            trace.push_back(CommandSMove{dx});
            position += dx;
        }
        auto dy = position.y < destination.y ? unitY : -unitY;
        while (position.y != destination.y) {
            trace.push_back(CommandSMove{dy});
            position += dy;
        }
    }
}
