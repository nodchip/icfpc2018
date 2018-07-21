#include "system.h"

#include <numeric>

#include "nanobot.h"

System::System(int R)
  : energy(0), harmonics_high(false), matrix(R) {
    Bot first_bot;
    first_bot.bid = 1;
    first_bot.pos = start_pos();
    // [2, 20]
    first_bot.seeds.resize(19);
    std::iota(first_bot.seeds.begin(), first_bot.seeds.end(), 2);
    bots = {first_bot};
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
