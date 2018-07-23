#pragma once

#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "matrix.h"
#include "nanobot.h"
#include "region.h"
#include "trace.h"
#include "vec3.h"
#include "union_find.h"

enum class EnergyTag {
    Halt, Wait, Flip, SMove, LMove,
    FillVoid, FillFull,
    VoidFull, VoidVoid,
    GFillVoid, GFillFull,
    GVoidFull, GVoidVoid,
    Fission, Fusion,
    HighHarmonics, LowHarmonics, Bots,
    N
};

struct EnergyLogger {
    virtual ~EnergyLogger() {}
    virtual void log_energy(EnergyTag tag, int64_t value) {}
};

struct AccumulateEnergyLogger : public EnergyLogger {
    AccumulateEnergyLogger() {
        for (int i = 0; i < int(EnergyTag::N); ++i) {
            consumption[i] = 0;
        }
    }
    void log_energy(EnergyTag tag, int64_t value) override {
        consumption[int(tag)] += value;
    }
    void dump(std::string output_path);
    std::map<int, int64_t> consumption;
};

constexpr int k_MaxNumberOfBots = 40;

struct System {
    explicit System(int R);

    void set_energy_logger(std::shared_ptr<EnergyLogger> energy_logger_) {
        log_energy = true;
        energy_logger = energy_logger_;
    }

    void global_energy_update();

    // @return true if halted.
    bool proceed_timestep();

    // @return bot index of bid.
    int bot_index_by(BotID bid) const;

    // @return bid (not index) at pos.
    int bid_at(Vec3 pos) const;

    void sort_by_bid();

    bool is_eof() const { return consumed_commands >= trace.size(); }

    // For debug
    void print();
    void print_detailed();
    void set_verbose(bool verbose_) { verbose = verbose_; }

    // record energy update with a tag.
    void add_energy(EnergyTag tag, int64_t value) {
        energy += value;
        if (log_energy) {
            energy_logger->log_energy(tag, value);
        }
    }

    // order-free bot command system.
    bool stage(const Bot& bot, Command cmd);
    bool stage(BotID bid, Command cmd);
    bool is_stage_filled() const;
    bool stage_all_unstaged(Command cmd = CommandWait{});
    bool reset_staged_commands();
    bool commit_commands();


    static Vec3 start_pos() { return Vec3(0, 0, 0); }
    static Vec3 final_pos() { return Vec3(0, 0, 0); }

    int64_t energy = 0;
    bool harmonics_high = false;
    Matrix matrix = {0};
    std::vector<Bot> bots;
    Trace trace;

    // (bid-1) => command
    std::vector<boost::optional<Command>> commands_stage;

    std::shared_ptr<EnergyLogger> energy_logger;
    bool log_energy = false;
    bool verbose = false;

    UnionFind ground_and_full_voxels;

    int64_t consumed_commands = 0; // not necessary for the game.
    int64_t timestep = 0; // not necessary for the game.
};
// vim: set si et sw=4 ts=4:
