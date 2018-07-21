#include "system.h"

#include <numeric>

#include "nanobot.h"

System::System(int R)
  : energy(0), harmonics_high(false), matrix(R)
  , ground_and_full_voxels(Vec3::index_end()) {
    Bot first_bot;
    first_bot.bid = 1;
    first_bot.pos = start_pos();
    // [2, 20]
    first_bot.seeds.resize(19);
    std::iota(first_bot.seeds.begin(), first_bot.seeds.end(), 2);
    bots = {first_bot};
    // unite the ground voxels. (y=0)
    for (int z = 0; z < R; ++z) {
        for (int x = 0; x < R; ++x) {
            ground_and_full_voxels.unionSet(
                Vec3(0, 0, 0).index(), Vec3(x, 0, z).index());
        }
    }
}

int System::bot_index_by(BotID bid) const {
    auto it = std::find_if(bots.begin(), bots.end(), [bid](const Bot& b) {
        return b.bid == bid;
    });
    if (it == bots.end()) return -1;
    return std::distance(bots.begin(), it);
}

int System::bid_at(Vec3 pos) const {
    auto it = std::find_if(bots.begin(), bots.end(), [pos](const Bot& b) {
        return b.pos == pos;
    });
    if (it == bots.end()) return -1;
    return it->bid;
}

void System::sort_by_bid() {
    std::sort(bots.begin(), bots.end(), [](const Bot& lhs, const Bot& rhs) {
        return lhs.bid < rhs.bid;
    });
}

void System::print() {
    std::cout << "System energy=" << energy
              << ", harmonics=" << (harmonics_high ? "high" : "low")
              << ", bots=" << bots.size()
              << ", trace=" << trace.size()
              << " commands=" << consumed_commands
              << std::endl;
}

void System::print_detailed() {
    print();
    for (size_t i = 0; i < bots.size(); ++i) {
        bots[i].print();
    }
}
// vim: set si et sw=4 ts=4:
