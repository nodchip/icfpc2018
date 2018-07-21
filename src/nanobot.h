#pragma once

#include <vector>
#include "vec3.h"

typedef uint32_t BotID;
struct Bot {
    BotID bid = 1;
    Vec3 pos = {0, 0, 0};
    std::vector<BotID> seeds;

    void print() {
        std::cout << "Bot#" << bid << " at " << pos << ", seeds=[";
        for (auto seed_bid : seeds) {
            std::cout << seed_bid << ", ";
        }
        std::cout << "]" << std::endl;
    }
};
