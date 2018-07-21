#pragma once

#include <vector>

#include "matrix.h"
#include "nanobot.h"
#include "region.h"
#include "trace.h"
#include "vec3.h"
#include "union_find.h"

struct System {
    explicit System(int R);

    void global_energy_update();

    // @return true if halted.
    bool proceed_timestep();

    // @return bot index of bid.
    int bot_index_by(BotID bid) const;

    // @return bid (not index) at pos.
    int bid_at(Vec3 pos) const;

    void sort_by_bid();

    // For debug
    void print();
    void print_detailed();

    static Vec3 start_pos() { return Vec3(0, 0, 0); }
    static Vec3 final_pos() { return Vec3(0, 0, 0); }

    int64_t energy = 0;
    bool harmonics_high = false;
    Matrix matrix = {0};
    std::vector<Bot> bots;
    Trace trace;

    UnionFind ground_and_full_voxels;

    int64_t consumed_commands = 0; // not necessary for the game.
};
// vim: set si et sw=4 ts=4:
