#include "nmms.h"
// std
#include <cstdio>
#include <map>
#include <unordered_map>
#include <numeric>
// 3rd
#include <gtest/gtest.h>

std::vector<Vec3> neighbors26() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x != 0 || y != 0 || z != 0) {
                    neighbors.emplace_back(x, y, z);
                }
            }
        }
    }
    return neighbors;
}

std::vector<Vec3> neighbors18() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x != 0 || y != 0 || z != 0) {
                    if (x == 0 || y == 0 || z == 0) {
                        neighbors.emplace_back(x, y, z);
                    }
                }
            }
        }
    }
    return neighbors;
}

std::vector<Vec3> neighbors6() {
    std::vector<Vec3> neighbors;
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if ((x != 0) + (y != 0) + (z != 0) == 1) {
                    neighbors.emplace_back(x, y, z);
                }
            }
        }
    }
    return neighbors;
}

namespace NOutputTrace {
    uint8_t nd_encoding(Vec3 nd) {
        // TODO: assert nd is a ND.
        return (nd.x + 1) * 9 + (nd.y + 1) * 3 + (nd.z + 1);
    }
    struct LDEncoding {
        static LDEncoding from_SLD(Vec3 ld) {
            // TODO: assert ld is a SLD.
            if (ld.x != 0) {
                return LDEncoding(0b01, ld.x + 5);
            } else if (ld.y != 0) {
                return LDEncoding(0b10, ld.y + 5);
            } else if (ld.z != 0) {
                return LDEncoding(0b11, ld.z + 5);
            }
            //ASSERT(false);
            return LDEncoding(0xff, 0xff);
        }
        static LDEncoding from_LLD(Vec3 ld) {
            // TODO: assert ld is a LLD.
            if (ld.x != 0) {
                return LDEncoding(0b01, ld.x + 15);
            } else if (ld.y != 0) {
                return LDEncoding(0b10, ld.y + 15);
            } else if (ld.z != 0) {
                return LDEncoding(0b11, ld.z + 15);
            }
            //ASSERT(false);
            return LDEncoding(0xff, 0xff);
        }
        uint8_t a = 0; // axis
        uint8_t i = 0; // length

    private:
        LDEncoding(uint8_t a_, uint8_t i_) : a(a_), i(i_) {}
    };

    struct EmitCommand : public boost::static_visitor<bool> {
        std::vector<uint8_t>& buffer;

        EmitCommand(std::vector<uint8_t>& buffer_)
            : buffer(buffer_) {}

        bool operator()(CommandHalt) { buffer.push_back(0b11111111); return true; }
        bool operator()(CommandWait) { buffer.push_back(0b11111110); return true; }
        bool operator()(CommandFlip) { buffer.push_back(0b11111101); return true; }
        bool operator()(CommandSMove cmd) {
            auto enc = LDEncoding::from_LLD(cmd.lld);
            buffer.push_back(0b00000100 | (enc.a << 4));
            buffer.push_back(enc.i);
            return true;
        };
        bool operator()(CommandLMove cmd) {
            auto enc1 = LDEncoding::from_SLD(cmd.sld1);
            auto enc2 = LDEncoding::from_SLD(cmd.sld2);
            buffer.push_back(0b00001100 | (enc2.a << 6) | (enc1.a << 4));
            buffer.push_back((enc2.i << 4) | enc1.i);
            return true;
        };
        bool operator()(CommandFission cmd) {
            //ASSERT(0 <= cmd.m && cmd.m <= 255);
            buffer.push_back(0b00000101 | (nd_encoding(cmd.nd) << 3));
            buffer.push_back(cmd.m);
            return true;
        };
        bool operator()(CommandFill cmd) {
            buffer.push_back(0b00000011 | (nd_encoding(cmd.nd) << 3));
            return true;
        };
        bool operator()(CommandFusionP cmd) {
            buffer.push_back(0b00000111 | (nd_encoding(cmd.nd) << 3));
            return true;
        };
        bool operator()(CommandFusionS cmd) {
            buffer.push_back(0b00000110 | (nd_encoding(cmd.nd) << 3));
            return true;
        };
    };
}

bool output_trace(std::string output_path, const Trace& trace) {
    std::vector<uint8_t> buf;
    NOutputTrace::EmitCommand visitor(buf);
    for (const auto& command : trace) {
        boost::apply_visitor(visitor, command);
    }
    
    std::FILE* fp = std::fopen(output_path.c_str(), "wb");
    std::fwrite(buf.data(), 1, buf.size(), fp);
    std::fclose(fp);

    return true;
}

Matrix load_model(std::string input_path) {
    std::FILE* fp = std::fopen(input_path.c_str(), "rb");

    uint8_t R = 0xff;
    std::fread(&R, 1, 1, fp);
    // TODO:check error, R!=0xff

    std::vector<uint8_t> buf((R * R * R + 8 - 1) / 8, 0);
    std::fread(buf.data(), 1, buf.size(), fp);
    std::fclose(fp);

    Matrix m(R);

    for (int z = 0; z < R; ++z) {
        for (int y = 0; y < R; ++y) {
            for (int x = 0; x < R; ++x) {
                const size_t bit = (x * R + y) * R + z;
                m(x, y, z) = (buf[bit >> 3] & (1 << (bit & 7))) ? Full : Void;
            }
        }
    }

    return m;
}

bool dump_model(std::string output_path, const Matrix& m) {
    // assert a valid m.
    std::vector<uint8_t> buf((m.R * m.R * m.R + 8 - 1) / 8, 0);
    for (int z = 0; z < m.R; ++z) {
        for (int y = 0; y < m.R; ++y) {
            for (int x = 0; x < m.R; ++x) {
                const size_t bit = (x * m.R + y) * m.R + z;
                if (m(x, y, z) != Void) {
                    buf[bit >> 3] |= (1 << (bit & 7));
                }
            }
        }
    }

    std::FILE* fp = std::fopen(output_path.c_str(), "wb");
    uint8_t R = m.R;
    std::fwrite(&R, 1, 1, fp);
    std::fwrite(buf.data(), 1, buf.size(), fp);
    std::fclose(fp);

    return true;
}

bool System::start(int R) {
    // TODO: assert R > 0

    energy = 0;
    harmonics_high = false;
    matrix = Matrix(R);

    Bot first_bot;
    first_bot.bid = 1;
    first_bot.pos = start_pos();
    // [2, 20]
    first_bot.seeds.resize(19);
    std::iota(first_bot.seeds.begin(), first_bot.seeds.end(), 2);
    bots = {first_bot};

    trace.clear();

    return true;
}

bool is_finished(const System& system, const Matrix& problem_matrix) {
    // TODO: check R.

    if (system.harmonics_high) return false;
    if (!system.bots.empty()) return false;
    // TODO: check trace == epsilon (???)
    if (system.matrix != problem_matrix) return false;

    return true;
}


namespace NProceedTimestep {

    constexpr int k_BotIDs = 21;
    struct FusionStage {
        FusionStage() {
            std::memset(fusion, 0, sizeof(int) * k_BotIDs * k_BotIDs);
        }

        void addPS(BotID me, BotID other) {
            fusion[me /* remaining */][other /* erased */] += 1;
        }
        void addSP(BotID me, BotID other) {
            fusion[other][me] += 1;
        }

        int fusion[k_BotIDs][k_BotIDs];

        bool update(System& sys) {
            for (int i = 0; i < k_BotIDs; ++i) {
                for (int j = 0; j < k_BotIDs; ++j) {
                    if (fusion[i][j]) {
                        if (fusion[i][j] != 2) return false;

                        const int idx_remain = sys.bot_index_by(i);
                        const int idx_erase = sys.bot_index_by(j);
                        if (idx_remain < 0 || idx_erase < 0) return false;

                        std::copy(sys.bots[idx_erase].seeds.begin(),
                             sys.bots[idx_erase].seeds.end(), 
                             std::back_inserter(sys.bots[idx_remain].seeds));
                        sys.bots[idx_remain].seeds.push_back(
                            sys.bots[idx_erase].bid);
                        sys.bots.erase(sys.bots.begin() + idx_erase);

                        sys.energy += Costs::k_Fusion;
                    }
                }
            }
            return true;
        }
    };

    struct UpdateSystem : public boost::static_visitor<bool> {
        System& sys;
        Bot& bot;
        bool& halt_requested;

        FusionStage& fusion_stage;

        UpdateSystem(System& sys_, Bot& bot_, bool& halt_requested_,
                FusionStage& fusion_stage_)
             : sys(sys_)
             , bot(bot_)
             , halt_requested(halt_requested_)
             , fusion_stage(fusion_stage_) {
             } 
        bool operator()(CommandHalt) { 
            if (sys.bots.size() != 1 || sys.bots[0].pos != sys.final_pos()) return false;
            halt_requested = true;
            return true;
        }
        bool operator()(CommandWait) {
             return true;
        }
        bool operator()(CommandFlip) { 
            // XXX: whether flipping twice in the same timestep is allowed is not clearly stated.
            sys.harmonics_high = !sys.harmonics_high;
            return true;
        }
        bool operator()(CommandSMove cmd) {
            if (!sys.matrix.is_in_matrix(bot.pos + cmd.lld)) return false;
            if (sys.matrix.any_full(Region(bot.pos, bot.pos + cmd.lld))) return false;
            bot.pos += cmd.lld;
            sys.energy += Costs::k_SMove * mlen(cmd.lld);
            return true;
        };
        bool operator()(CommandLMove cmd) {
            auto c1 = bot.pos + cmd.sld1;
            auto c2 = c1 + cmd.sld2;
            if (!sys.matrix.is_in_matrix(c1)) return false;
            if (!sys.matrix.is_in_matrix(c2)) return false;
            if (sys.matrix.any_full(Region(bot.pos, c1))) return false;
            if (sys.matrix.any_full(Region(c1, c2))) return false;
            bot.pos = c2;
            sys.energy += Costs::k_LMove * (mlen(cmd.sld1) + Costs::k_LMoveOffset + mlen(cmd.sld2));
            return true;
        };
        bool operator()(CommandFill cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) return false;
            if (sys.matrix(c) == Void) {
                sys.matrix(c) = Full;
                sys.energy += Costs::k_FillVoid;
            } else {
                sys.energy += Costs::k_FillFull;
            }
            return true;
        };
        bool operator()(CommandFission cmd) {
            auto c = bot.pos + cmd.nd;
            const int n_bots = int(bot.seeds.size());
            if (n_bots == 0 || n_bots <= cmd.m || cmd.m < 0) return false;
            if (!sys.matrix.is_in_matrix(c)) return false;
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
            sys.energy += Costs::k_Fission;
            return true;
        };
        bool operator()(CommandFusionP cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) return false;
            fusion_stage.addPS(bot.bid, sys.bid_at(c));
            return true;
        };
        bool operator()(CommandFusionS cmd) {
            auto c = bot.pos + cmd.nd;
            if (!sys.matrix.is_in_matrix(c)) return false;
            fusion_stage.addSP(bot.bid, sys.bid_at(c));
            return true;
        };
    };
}

TEST(Commands, Fission) {
    System system;
    system.start(4);
    int m = 2;

    bool halt = false;
    // fission
    {
        NProceedTimestep::FusionStage fusion_stage;
        NProceedTimestep::UpdateSystem visitor(system, system.bots[0], halt, fusion_stage);
        Command cmd = CommandFission{Vec3 {0, 0, 1}, m};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }

    system.print_detailed();

    ASSERT_EQ(system.bots.size(), 2);
    EXPECT_EQ(system.bots[0].seeds.size(), 18 - m);
    EXPECT_EQ(system.bots[1].seeds.size(), m);

    // fusion.
    NProceedTimestep::FusionStage fusion_stage;
    {
        NProceedTimestep::UpdateSystem visitor(system, system.bots[0], halt, fusion_stage);
        Command cmd = CommandFusionS{Vec3 {0, 0, 1}};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }
    {
        NProceedTimestep::UpdateSystem visitor(system, system.bots[1], halt, fusion_stage);
        Command cmd = CommandFusionP{Vec3 {0, 0, -1}};
        EXPECT_TRUE(boost::apply_visitor(visitor, cmd));
    }
    EXPECT_TRUE(fusion_stage.update(system));

    system.print_detailed();

    ASSERT_EQ(system.bots.size(), 1);
    EXPECT_EQ(system.bots[0].seeds.size(), 19);
}

bool proceed_timestep(System& system) {
    bool halt = false;

    NProceedTimestep::FusionStage fusion_stage;

    const size_t n = system.bots.size();
    for (size_t i = 0; i < n; ++i) {
        Command cmd = system.trace.front(); system.trace.pop_front();
        ++system.consumed_commands;

        NProceedTimestep::UpdateSystem visitor(system, system.bots[i], halt, fusion_stage);
        if (!boost::apply_visitor(visitor, cmd)) {
            throw std::runtime_error("wrong command");
        }
    }

    fusion_stage.update(system);

    if (halt) {
        system.bots.clear();
    }

    return halt;
}

bool simulate_all(System& system) {
    while (!system.trace.empty()) {
        if (proceed_timestep(system)) {
            return true;
        }
    }
    return false;
}

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

// vim: set si et sw=4 ts=4:
