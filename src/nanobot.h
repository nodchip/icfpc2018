#pragma once

typedef uint32_t BotID;
struct Bot {
    BotID bid = 1;
    Vec3 pos = {0, 0, 0};
    std::vector<BotID> seeds;

    void print() {
        std::printf("Bot#%d at (%d, %d, %d), seeds=[",
            bid, pos.x, pos.y, pos.z);
        for (auto seed_bid : seeds) {
            std::printf("%d, ", seed_bid);
        }
        std::printf("]\n");
    }
};
