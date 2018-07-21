#include "nmms.h"
// std
#include <map>
#include <unordered_map>
// project
#include "debug_message.h"

bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt) {
    if (m(start_pos) || m(stop_pos)) {
        return false;
    }
    if (start_pos == stop_pos) {
        return true;
    }

    std::deque<Vec3> queue;
    std::unordered_map<Vec3, Vec3, Vec3::hash> parent;
    Matrix blocked = m; // initially Full voxels are blocked.
    queue.push_back(stop_pos);
    blocked(stop_pos) = Full;

    auto n6 = neighbors6();

    while (!queue.empty()) {
        auto p = queue.front(); queue.pop_front();
        for (auto offset : n6) {
            auto n = p + offset;
            if (m.is_in_matrix(n) && !blocked(n)) {
                parent[n] = p;
                if (n == start_pos) {
                    // backtrack.
                    auto cursor = n;
                    do {
                        auto par = parent[cursor];
                        if (trace_opt) {
                            trace_opt->push_back(CommandSMove{par - cursor});
                        }
                        if (trajectory_opt) {
                            trajectory_opt->push_back(par);
                        }
                        cursor = par;
                    } while (cursor != stop_pos);
                    return true;
                }
                queue.push_back(n);
                blocked(n) = Full;
            }
        }
    }

    return false;
}

bool fast_manhattan_motion_in_void(const Matrix& matrix, Vec3 start_pos, Vec3 stop_pos,
    Trace& trace) {
    ASSERT_ERROR_RETURN(matrix.is_in_matrix(start_pos), false);
    ASSERT_ERROR_RETURN(matrix.is_in_matrix(stop_pos), false);

    const int sx = start_pos.x, sy = start_pos.y, sz = start_pos.z;
    const int dx = stop_pos.x, dy = stop_pos.y, dz = stop_pos.z;
    
    Vec3 corner[8] = {
        Vec3(sx, sy, sz), Vec3(dx, sy, sz),
        Vec3(sx, dy, sz), Vec3(dx, dy, sz),
        Vec3(sx, sy, dz), Vec3(dx, sy, dz),
        Vec3(sx, dy, dz), Vec3(dx, dy, dz),
    };
    int idxs[6][2] = {{1, 3}, {1, 5}, {4, 5}, {4, 6}, {2, 3}, {2, 6}};
    for (int i = 0; i < 6; ++i) {
        // 0, idxs[i][0], idxs[i][1], 7
        int p0 = idxs[i][0];
        int p1 = idxs[i][1];
        if (!matrix.any_full(Region(corner[0], corner[p0])) && 
            !matrix.any_full(Region(corner[p1], corner[7]))) {
            // found
            fast_move(corner[p0], corner[0], trace);
            fast_move(corner[p1], corner[p0], trace);
            fast_move(corner[7], corner[p1], trace);
            return true;
        }
    }
    return false;
}

constexpr int k_LLDLength = 15;
constexpr int k_SLDLength = 5;
void fast_move(const Vec3& destination, Vec3& position, Trace& trace) {
    const Vec3 unitX(1, 0, 0);
    const Vec3 unitY(0, 1, 0);
    const Vec3 unitZ(0, 0, 1);

    const Vec3 dx = (destination.x > position.x) ? unitX : -unitX;
    while (position.x != destination.x) {
        const int diff = std::min(k_LLDLength, std::abs(position.x - destination.x));
        trace.push_back(CommandSMove{dx * diff});
        position += dx * diff;
    }
    const Vec3 dy = (destination.y > position.y) ? unitY : -unitY;
    while (position.y != destination.y) {
        const int diff = std::min(k_LLDLength, std::abs(position.y - destination.y));
        trace.push_back(CommandSMove{dy * diff});
        position += dy * diff;
    }
    const Vec3 dz = (destination.z > position.z) ? unitZ : -unitZ;
    while (position.z != destination.z) {
        const int diff = std::min(k_LLDLength, std::abs(position.z - destination.z));
        trace.push_back(CommandSMove{dz * diff});
        position += dz * diff;
    }
}

// vim: set si et sw=4 ts=4:
