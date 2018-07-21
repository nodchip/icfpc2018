// Nanobot Matter Manipulating System
//
#ifndef __NMMS_H__
#define __NMMS_H__

#include <vector>
#include <deque>
#include <memory>
#include <boost/variant.hpp>

#include "vec3.h"

struct Region {
    Region(Vec3 c1_, Vec3 c2_) : c1(c1_), c2(c2_) {}
    Vec3 c1;
    Vec3 c2;
    bool operator==(const Region& rhs) const {
        Region r = rhs.canonical();
        return c1 == r.c1 && c2 == r.c2;
    }
    Region canonical() const {
        return Region {
            Vec3(std::min(c1.x, c2.x), std::min(c1.y, c2.y), std::min(c1.z, c2.z)),
            Vec3(std::max(c1.x, c2.x), std::max(c1.y, c2.y), std::max(c1.z, c2.z))
            };
    }
};


struct CommandHalt {};
struct CommandWait {};
struct CommandFlip {};
struct CommandSMove {
    Vec3 lld;
};
struct CommandLMove {
    Vec3 sld1;
    Vec3 sld2;
};
struct CommandFission {
    Vec3 nd;
    int m;
};
struct CommandFill {
    Vec3 nd;
};
struct CommandFusionP {
    Vec3 nd;
};
struct CommandFusionS {
    Vec3 nd;
};

namespace Costs {
    // global
    constexpr int k_HighHarmonics = 30;
    constexpr int k_LowHarmonics = 3;
    constexpr int k_Bot = 20;

    // command
    constexpr int k_SMove = 2;
    constexpr int k_LMove = 2;
    constexpr int k_LMoveOffset = 2;
    constexpr int k_FillVoid = 12;
    constexpr int k_FillFull = 6;
    constexpr int k_Fission = 24;
    constexpr int k_Fusion = -24;
}

typedef boost::variant<
    CommandHalt,
    CommandWait,
    CommandFlip,
    CommandSMove,
    CommandLMove,
    CommandFission,
    CommandFill,
    CommandFusionP,
    CommandFusionS> Command;

typedef std::deque<Command> Trace;
bool output_trace(std::string output_path, const Trace& trace);


enum { Void = 0, Full = 1 };
struct Matrix {
    Matrix(int R_) : R(R_), buf(R*R*R, Void) {
    }

    uint8_t& operator()(int x, int y, int z) {
        return buf[(z * R + y) * R + x];
    }
    uint8_t operator()(int x, int y, int z) const {
        return buf[(z * R + y) * R + x];
    }
    uint8_t& operator()(const Vec3& p) { return operator()(p.x, p.y, p.z); }
    uint8_t operator()(const Vec3& p) const { return operator()(p.x, p.y, p.z); }

    // is this a valid matrix?
    operator bool() const {
        return 0 < R && !buf.empty();
    }

    bool is_in_matrix(int x, int y, int z) const {
         return (0 <= x && x < R) && (0 <= y && y < R) && (0 <= z && z < R);
    }
    bool is_in_matrix(const Vec3& p) const { return is_in_matrix(p.x, p.y, p.z); }

    bool any_full(Region r) const {
        r = r.canonical();
        for (int z = r.c1.z; z <= r.c2.z; ++z) {
            for (int y = r.c1.y; y <= r.c2.y; ++y) {
                for (int x = r.c1.x; x <= r.c2.x; ++x) {
                    if ((*this)(x, y, z)) return true;
                }
            }
        }
        return false;
    }

    int R;
    std::vector<uint8_t> buf;
};
typedef std::shared_ptr<Matrix> MatrixPtr;

Matrix load_model(std::string input_path);
bool dump_model(std::string output_path, const Matrix& m);


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

// @return well-formed state and matters are identical to problem_matrix.
//         i.e. ready to submit.
bool is_finished(const System& system, const Matrix& problem_matrix);

void global_energy_update(System& system);

// @return true if halted.
bool proceed_timestep(System& system);

// @return true if halted.
bool simulate_all(System& system);

// 6neighbors
bool bfs_shortest_in_void(const Matrix& m, Vec3 start_pos, Vec3 stop_pos,
    Trace* trace_opt, std::vector<Vec3>* trajectory_opt);

#endif // __NMMS_H__
// vim: set si et sw=4 ts=4:
