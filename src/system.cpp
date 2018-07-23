#include "system.h"

#include <fstream>
#include <map>
#include <numeric>
#include <nlohmann/json.hpp>

#include "command.h"
#include "log.h"
#include "nanobot.h"

namespace NProceedTimestep {

struct GroundConnectivityChecker {
    std::vector<Vec3> filled_voxels;

    void fill(Vec3 p) {
        filled_voxels.push_back(p);
    }

    bool new_voxels_are_grounded(/* no const for union find */System& system) {
        const uint32_t ground_group = Vec3(0, 0, 0).index();
        for (auto& p : filled_voxels) {
            bool grounded = false;
            for (const auto& offset : neighbors6) {
                auto c = p + offset;
                if (system.matrix.is_in_matrix(c)) {
                    if (system.ground_and_full_voxels.findSet(c.index(), ground_group)) {
                        grounded = true;
                        break;
                    }
                }
            }
            if (!grounded) {
                return false;
            }
        }
        return true;
    }

    bool update(System& system) {
        for (auto& p : filled_voxels) {
            for (const auto& offset : neighbors6) {
                auto c = p + offset;
                if (system.matrix(c)) {
                    system.ground_and_full_voxels.unionSet(p.index(), c.index());
                }
            }
        }
        return true;
    }
};

struct FusionStage {
    static constexpr int kMaxBots = 100;
    map<int,int> fusion_map;

    FusionStage() {}

    void addPS(BotID me, BotID other) {
        const int index = me * kMaxBots + other;
        fusion_map[index] = fusion_map[index] + 1;
    }

    void addSP(BotID me, BotID other) {
        const int index = other * kMaxBots + me;
        fusion_map[index] = fusion_map[index] + 1;
    }

    bool update(System& sys) {
        for (auto& p : fusion_map) {
            if (p.second != 2) {
                LOG() << "Inconsistent FusionState";
                return false;
            }

            int i = p.first / kMaxBots;
            int j = p.first % kMaxBots;
            const int idx_remain = sys.bot_index_by(i);
            const int idx_erase = sys.bot_index_by(j);
            if (idx_remain < 0 || idx_erase < 0)
                return false;

            std::copy(sys.bots[idx_erase].seeds.begin(),
                      sys.bots[idx_erase].seeds.end(),
                      std::back_inserter(sys.bots[idx_remain].seeds));
            sys.bots[idx_remain].seeds.push_back(sys.bots[idx_erase].bid);
            sys.bots.erase(sys.bots.begin() + idx_erase);

            sys.add_energy(EnergyTag::Fusion, Costs::k_Fusion);
        }
        return true;
    }
};

struct GroupStage {
    enum {ACTION_FILL, ACTION_VOID};
    // canonical Region. => [bid]
    std::unordered_map<Region, std::vector<BotID>> stage[2];

    GroupStage() {}

    bool add_bot(Bot& bot, Vec3 nd, Vec3 fd, int action_type) {
        ASSERT_RETURN(is_valid_nd(nd), false);
        ASSERT_RETURN(is_valid_fd(fd), false);
        auto r = Region(bot.pos + nd, bot.pos + nd + fd).canonical();
        stage[action_type][r].push_back(bot.bid);
        return true;
    }

    bool update(System& sys, int action_type) {
        for (auto it = stage[action_type].begin(); it != stage[action_type].end(); ++it) {
            const Region& r = it->first;
            ASSERT_RETURN(sys.matrix.is_in_matrix(r.c1) & sys.matrix.is_in_matrix(r.c2), false);
            const auto& bids = it->second;
            const int nbots = bids.size();
            if (false) {
                LOG() << "BIDS"; 
                for (auto b : bids) {
                    std::cerr << b << " ";
                }
                std::cerr << std::endl;
                LOG() << "region " << r.c1 << " " << r.c2 << std::endl;
            }
            ASSERT_RETURN(nbots == 2 || nbots == 4 || nbots == 8, false);
            for (auto bid : bids) {
                auto& bot = sys.bots[sys.bot_index_by(bid)];
                ASSERT_RETURN(!r.is_in_region(bot.pos), false);
            }

            int64_t fillvoid = 0, fillfull = 0, voidfull = 0, voidvoid = 0;
            CANONICAL_REGION_FOR(it->first, x, y, z) {
                if (action_type == ACTION_FILL) {
                   if (sys.matrix(x, y, z) == Void) {
                       // XXX: ground check..
                       sys.matrix(x, y, z) = Full;
                       fillvoid += Costs::k_GFillVoid;
                   } else {
                       fillfull += Costs::k_GFillFull;
                   }
                } else {
                   if (sys.matrix(x, y, z) == Full) {
                       // XXX: ground check..
                       sys.matrix(x, y, z) = Void;
                       voidfull += Costs::k_GVoidFull;
                   } else {
                       voidvoid += Costs::k_GVoidVoid;
                   }
                }
            }
            if (fillvoid != 0) { sys.add_energy(EnergyTag::GFillVoid, fillvoid); }
            if (fillfull != 0) { sys.add_energy(EnergyTag::GFillFull, fillfull); }
            if (voidfull != 0) { sys.add_energy(EnergyTag::GVoidFull, voidfull); }
            if (voidvoid != 0) { sys.add_energy(EnergyTag::GVoidVoid, voidvoid); }
        }

        return true;
    }
};

struct UpdateSystem : public boost::static_visitor<bool> {
    System& sys;
    Bot& bot;
    bool& halt_requested;

    FusionStage& fusion_stage;
    GroupStage& group_stage;
    GroundConnectivityChecker& ground_connectivity_checker;

    UpdateSystem(System& sys_, Bot& bot_, bool& halt_requested_,
                 FusionStage& fusion_stage_,
                 GroupStage& group_stage_,
                 GroundConnectivityChecker& ground_connectivity_checker_)
             : sys(sys_)
             , bot(bot_)
             , halt_requested(halt_requested_)
             , fusion_stage(fusion_stage_)
             , group_stage(group_stage_)
             , ground_connectivity_checker(ground_connectivity_checker_) {
    }

    bool operator()(CommandHalt) {
        if (sys.bots.size() != 1 || sys.bots[0].pos != sys.final_pos()) {
            LOG() << "[CommandHalt] preconditions are not met.";
            sys.bots[0].pos.print();
            return false;
        }
        halt_requested = true;
        return true;
    }
    bool operator()(CommandWait) {
        return true;
    }
    bool operator()(CommandFlip) {
        sys.harmonics_high = !sys.harmonics_high;
        return true;
    }
    bool operator()(CommandSMove cmd) {
        if (!sys.matrix.is_in_matrix(bot.pos + cmd.lld)) {
            LOG() << "[CommandSMove] target position out of range.";
            bot.pos.print();
            cmd.lld.print();
            return false;
        }
        if (sys.matrix.any_full(Region(bot.pos, bot.pos + cmd.lld))) {
            LOG() << "[CommandSMove] some full voxels in between the move.";
            bot.pos.print();
            cmd.lld.print();
            return false;
        }
        bot.pos += cmd.lld;
        sys.add_energy(EnergyTag::SMove, Costs::k_SMove * mlen(cmd.lld));
        return true;
    };
    bool operator()(CommandLMove cmd) {
        auto c1 = bot.pos + cmd.sld1;
        auto c2 = c1 + cmd.sld2;
        if (!sys.matrix.is_in_matrix(c1)) {
            LOG() << "[CommandLMove] c1 out of range";
            return false;
        }
        if (!sys.matrix.is_in_matrix(c2)) {
            LOG() << "[CommandLMove] c2 out of range";
            return false;
        }
        if (sys.matrix.any_full(Region(bot.pos, c1))) {
            LOG() << "[CommandLMove] some full voxels in between the move.";
            return false;
        }
        if (sys.matrix.any_full(Region(c1, c2))) {
            LOG() << "[CommandLMove] some full voxels in between the move.";
            return false;
        }
        bot.pos = c2;
        sys.add_energy(EnergyTag::LMove, Costs::k_LMove * (mlen(cmd.sld1) + Costs::k_LMoveOffset + mlen(cmd.sld2)));
        return true;
    };
    bool operator()(CommandFill cmd) {
        auto c = bot.pos + cmd.nd;
        if (!sys.matrix.is_in_matrix(c)) {
            LOG() << "[CommandFill] target voxel out of range";
            return false;
        }
        if (sys.matrix(c) == Void) {
            sys.matrix(c) = Full;
            ground_connectivity_checker.fill(c);
            sys.add_energy(EnergyTag::FillVoid, Costs::k_FillVoid);
        } else {
            sys.add_energy(EnergyTag::FillFull, Costs::k_FillFull);
        }
        return true;
    };
    bool operator()(CommandVoid cmd) {
        auto c = bot.pos + cmd.nd;
        if (!sys.matrix.is_in_matrix(c)) {
            LOG() << "[CommandVoid] target voxel out of range";
            return false;
        }
        if (sys.matrix(c) == Full) {
            sys.matrix(c) = Void;
            // XXX: Voiding violates the assumption of union-find...
            // ground_connectivity_checker.fill(c);
            sys.add_energy(EnergyTag::VoidFull, Costs::k_VoidFull);
        } else {
            sys.add_energy(EnergyTag::VoidVoid, Costs::k_VoidVoid);
        }
        return true;
    };
    bool operator()(CommandGFill cmd) {
        return group_stage.add_bot(bot, cmd.nd, cmd.fd, GroupStage::ACTION_FILL);
    };
    bool operator()(CommandGVoid cmd) {
        return group_stage.add_bot(bot, cmd.nd, cmd.fd, GroupStage::ACTION_VOID);
    };
    bool operator()(CommandFission cmd) {
        auto c = bot.pos + cmd.nd;
        const int n_bots = int(bot.seeds.size());
        if (n_bots == 0 || n_bots <= cmd.m || cmd.m < 0) {
            LOG() << "[CommandFission] preconditions are not met\n";
            return false;
        }
        if (!sys.matrix.is_in_matrix(c)) {
            LOG() << "[CommandFission] target voxel out of range.\n";
            return false;
        }
        // original  [bid1, bid2, .. bidm, bidm+1, .. bidn]
        // new bot    bid1
        // new seed        [bid2, .. bidm]
        // orig.seed                       [bidm+1, .. bidn]
        std::sort(bot.seeds.begin(), bot.seeds.end());
        auto it = bot.seeds.begin();
        Bot new_bot = {*it, c, {}};
        it = bot.seeds.erase(it);
        while (int(new_bot.seeds.size()) < cmd.m) {
            new_bot.seeds.push_back(*it);
            it = bot.seeds.erase(it);
        }
        sys.bots.push_back(new_bot);
        sys.add_energy(EnergyTag::Fission, Costs::k_Fission);
        return true;
    };
    bool operator()(CommandFusionP cmd) {
        auto c = bot.pos + cmd.nd;
        if (!sys.matrix.is_in_matrix(c)) {
            LOG() << "[CommandFusionP] target voxel out of range";
            return false;
        }
        fusion_stage.addPS(bot.bid, sys.bid_at(c));
        return true;
    };
    bool operator()(CommandFusionS cmd) {
        auto c = bot.pos + cmd.nd;
        if (!sys.matrix.is_in_matrix(c)) {
            LOG() << "[CommandFusionS] target voxel out of range";
            return false;
        }
        fusion_stage.addSP(bot.bid, sys.bid_at(c));
        return true;
    };
    bool operator()(CommandDebugMoveTo cmd) {
        if (!sys.matrix.is_in_matrix(cmd.pos)) {
            LOG() << "[CommandDebugMoveTo] target voxel out of range";
            return false;
        }
        if (!sys.matrix(cmd.pos)) {
            LOG() << "[CommandDebugMoveTo] target voxel occupied";
            return false;
        }
        bot.pos = cmd.pos;
        return true;
    };
};

}  // namespace NProceedTimestep

void AccumulateEnergyLogger::dump(std::string output_path) {
    std::string names[] = {
        "Halt", "Wait", "Flip", "SMove", "LMove",
        "FillVoid", "FillFull",
        "VoidFull", "VoidVoid",
        "GFillVoid", "GFillFull",
        "GVoidFull", "GVoidVoid",
        "Fission", "Fusion",
        "HighHarmonics", "LowHarmonics", "Bots",
    };
    nlohmann::json j = {};
    for (int i = 0; i < int(EnergyTag::N); ++i) {
        j[names[i]] = consumption[i];
    }

    std::ofstream ofs(output_path);
    ofs << j.dump(4);
}

System::System(int R)
  : energy(0), harmonics_high(false), matrix(R)
  , ground_and_full_voxels(Vec3::index_end()) {
    Bot first_bot;
    first_bot.bid = 1;
    first_bot.pos = start_pos();
    // [2, k_MaxNumberOfBots]
    first_bot.seeds.resize(k_MaxNumberOfBots - 1);
    std::iota(first_bot.seeds.begin(), first_bot.seeds.end(), 2);
    bots = {first_bot};
    // staging area
    commands_stage.assign(k_MaxNumberOfBots, boost::none);
    // unite the ground voxels. (y=0)
    for (int z = 0; z < R; ++z) {
        for (int x = 0; x < R; ++x) {
            ground_and_full_voxels.unionSet(
                Vec3(0, 0, 0).index(), Vec3(x, 0, z).index());
        }
    }
}

void System::global_energy_update() {
    if (harmonics_high) {
        add_energy(EnergyTag::HighHarmonics, Costs::k_HighHarmonics * matrix.R * matrix.R * matrix.R);
    } else {
        add_energy(EnergyTag::LowHarmonics, Costs::k_LowHarmonics * matrix.R * matrix.R * matrix.R);
    }
    add_energy(EnergyTag::Bots, Costs::k_Bot * bots.size());
}

bool System::proceed_timestep() {
    bool halt = false;
    NProceedTimestep::FusionStage fusion_stage;
    NProceedTimestep::GroupStage group_stage;
    NProceedTimestep::GroundConnectivityChecker ground_connectivity_checker;

    // systemwide energy (count the nuber of bots before fusion/fission).
    global_energy_update();

    // bots consume trace in the ascending order of bids.
    sort_by_bid();

    if (verbose) {
        std::printf("------------[timestep: %7ld]\n", timestep);
    }

    const size_t n = bots.size();
    for (size_t i = 0; i < n; ++i) {
        Command& cmd = trace[consumed_commands++];

        if (verbose) {
            std::printf("%8ld : ", consumed_commands);
            PrintCommand visitor(std::cout);
            boost::apply_visitor(visitor, cmd);
            std::printf("\n");
        }

        NProceedTimestep::UpdateSystem visitor(*this, bots[i], halt, fusion_stage, group_stage, ground_connectivity_checker);
        if (!boost::apply_visitor(visitor, cmd)) {
            LOG() << "Error while processing trace for bot " << i << "\n";
            print_detailed();
            throw std::runtime_error("wrong command");
	    // if you want trace data...
	    // return true;
        }

    }

    fusion_stage.update(*this);
    group_stage.update(*this, NProceedTimestep::GroupStage::ACTION_FILL);
    group_stage.update(*this, NProceedTimestep::GroupStage::ACTION_VOID);

    // if there are any floating voxels added in this phase while harmonics is low,
    // it is ill-formed.
    if (!harmonics_high && !ground_connectivity_checker.new_voxels_are_grounded(*this)) {
        LOG() << "Detected violation in ground connectivity in low-harmonics mode!\n";
    }
    ground_connectivity_checker.update(*this);

    if (halt) {
        bots.clear();
    }

    if (verbose) {
        print_detailed();
    }

    ++timestep;
    return halt;
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
              << ", R=" << matrix.R
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


bool System::stage(const Bot& bot, Command cmd) {
    ASSERT_RETURN(0 <= bot.bid - 1 && bot.bid - 1 < commands_stage.size(), false);
    commands_stage[bot.bid - 1] = cmd;
    return true;
}

bool System::stage(BotID bid, Command cmd) {
    ASSERT_RETURN(0 <= bid - 1 && bid - 1 < commands_stage.size(), false);
    commands_stage[bid - 1] = cmd;
    return true;
}

bool System::is_stage_filled() const {
    return bots.size() == std::count_if(commands_stage.begin(), commands_stage.end(), [](auto i) {
        return bool(i);
    });
}

bool System::stage_all_unstaged(Command cmd) {
    for (size_t i = 0; i < bots.size(); ++i) {
        if (!commands_stage[bots[i].bid - 1]) {
            commands_stage[bots[i].bid - 1] = cmd;
        }
    }
    return true;
}

bool System::reset_staged_commands() {
    commands_stage.assign(commands_stage.size(), boost::none);
    return true;
}

bool System::commit_commands() {
    ASSERT_RETURN(is_stage_filled(), false);

    // naturally sorted in the ascending order of bid.
    for (size_t i = 0; i < commands_stage.size(); ++i) {
        if (commands_stage[i]) {
            trace.push_back(*commands_stage[i]);
        }
    }
    reset_staged_commands();

    proceed_timestep();

    return true;
}
