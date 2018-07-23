#include "naive_move.h"

#include "../trace.h"
#include "../vec3.h"

static const Vec3 unitX(1, 0, 0);
static const Vec3 unitY(0, 1, 0);
static const Vec3 unitZ(0, 0, 1);

void naive_move(const Vec3& destination, Vec3& position, Trace& trace) {
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
    temp_trace.reduction_smove();
    trace.insert(trace.end(), temp_trace.begin(), temp_trace.end());
}
