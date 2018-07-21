#pragma once

#include <vector>

#include "matrix.h"
#include "nanobot.h"
#include "region.h"
#include "trace.h"
#include "vec3.h"

struct System {
    int64_t energy = 0;
    bool harmonics_high = false;
    Matrix matrix = {0};
    std::vector<Bot> bots;
    Trace trace;

    int64_t consumed_commands = 0; // not necessary for the game.

    static Vec3 start_pos() { return Vec3(0, 0, 0); }
    static Vec3 final_pos() { return Vec3(0, 0, 0); }

    bool start(int R);

    // @return bot index of bid.
    int bot_index_by(BotID bid) const {
        auto it = std::find_if(bots.begin(), bots.end(), [bid](const Bot& b) {
            return b.bid == bid;
        });
        if (it == bots.end()) return -1;
        return std::distance(bots.begin(), it);
    }

    // @return bid (not index) at pos.
    int bid_at(Vec3 pos) const {
        auto it = std::find_if(bots.begin(), bots.end(), [pos](const Bot& b) {
            return b.pos == pos;
        });
        if (it == bots.end()) return -1;
        return it->bid;
    }

    void sort_by_bid() {
        std::sort(bots.begin(), bots.end(), [](const Bot& lhs, const Bot& rhs) {
            return lhs.bid < rhs.bid;
        });
    }

    void print() {
        std::printf("System energy=%ld, harmonics=%s, bots=%ld, trace=%ld commands=%ld\n",
            energy, harmonics_high ? "high" : "low",
            bots.size(), trace.size(), consumed_commands);
    }
    void print_detailed() {
        print();
        for (size_t i = 0; i < bots.size(); ++i) {
            bots[i].print();
        }
    }
};
