// frequently used? traces
#pragma once

#include <vector>

#include "log.h"
#include "trace.h"
#include "matrix.h"
#include "system.h"

namespace NTraceUtil {

// X-aligned linear position distribution for easy merge.
std::vector<Vec3> create_x_linear_positions(const Matrix& blocked, int n_nanobots);

// fusion [(0, 0, 0), (1, 0, 0), .. ] => (0, 0, 0)
// @note  positions are sorted by bid.
bool fusion_x_linear_positions_to_first_pos(const std::vector<Vec3>& positions, Trace& trace);

// [0] [2] [4] ..
bool fission_x_2_linear_positions(Vec3 start_pos, int N, int R, std::vector<Vec3>& id_to_pos, Trace& trace);
bool fission_x_2by2_linear_positions(Vec3 start_pos, int N, int R, std::vector<Vec3>& id_to_pos, Trace& trace);

template <typename T>
bool sort_by_bid(std::vector<T>& container, const std::vector<Bot>& bots) {
    ASSERT_RETURN(container.size() == bots.size(), false);
    std::vector<std::pair<BotID, size_t>> bids;
    for (size_t i = 0; i < bots.size(); ++i) {
        bids.emplace_back(bots[i].bid, i);
    }
    std::sort(bids.begin(), bids.end(), [](auto lhs, auto rhs) { return lhs.first < rhs.first; });
    std::vector<T> res;
    for (size_t i = 0; i < bots.size(); ++i) {
        res.push_back(container[bids[i].second]);
    }
    container.swap(res);
    return true;
}

// not sorted.
std::vector<Vec3> bot_positions(const System& system);

// Wait commands are appended if the trace lengths are not equal.
// no fusion/fission supported.
// @note  traces are sorted by bid.
bool merge_traces(const std::vector<Trace>& traces, Trace& trace);

// greedy & conservative (any successive bots can not pass through another bot's trajectory).
// TODO: ignore other bots first, then merge trace using inserting Wait if required. 
// @note  positions are sorted by bid.
#if 0
bool multibot_merge_conservative(const Matrix& blocked, const std::vector<Vec3>& start_positions,
        const std::vector<Vec3>& final_positions, Trace& trace);
#endif

// @note  positions are sorted by bid.
bool move_evacuated_multibots(const Matrix& blocked, const std::vector<Vec3>& start_positions,
        const std::vector<Vec3>& final_positions, Trace& trace);

bool digging_evacuate(const Matrix& blocked, Vec3& position, Trace& trace);

}
