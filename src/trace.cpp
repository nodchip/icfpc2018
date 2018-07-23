#include "trace.h"

#include <vector>
#include <iomanip>
#include <fstream>
#include <nlohmann/json.hpp>

#include "log.h"

namespace {
Vec3 getLLD(Vec3 &in) {
    Vec3 out(0, 0, 0);
    if (in[0] > 0) {
        out[0] = std::min<int>(in[0], 15);
        in[0] -= out[0];
    } else if (in[1] > 0) {
        out[1] = std::min<int>(in[1], 15);
        in[1] -= out[1];
    } else if (in[2] > 0) {
        out[2] = std::min<int>(in[2], 15);
        in[2] -= out[2];
    } else if (in[0] < 0) {
        out[0] = std::max<int>(in[0], -15);
        in[0] -= out[0];
    } else if (in[1] < 0) {
        out[1] = std::max<int>(in[1], -15);
        in[1] -= out[1];
    } else if (in[2] < 0) {
        out[2] = std::max<int>(in[2], -15);
        in[2] -= out[2];
    }
    return out;
}

};

namespace NOutputTrace {

uint8_t nd_encoding(Vec3 nd) {
    ASSERT(is_valid_nd(nd));
    return (nd.x + 1) * 9 + (nd.y + 1) * 3 + (nd.z + 1);
}
Vec3 nd_decoding(uint8_t b) {
    int z = b % 3 - 1;
    int y = b / 3 % 3 - 1;
    int x = b / 9 - 1;
    return Vec3(x, y, z);
}
std::array<uint8_t, 3> fd_encoding(Vec3 fd) {
    ASSERT(is_valid_fd(fd));
    return {uint8_t(fd.x + 30), uint8_t(fd.y + 30), uint8_t(fd.z + 30)};
}
Vec3 fd_decoding(uint8_t b0, uint8_t b1, uint8_t b2) {
    return Vec3(int(b0) - 30, int(b1) - 30, int(b2) - 30);
}

struct LDDecoding {
    static Vec3 from_SLD(uint8_t a, uint8_t i) {
        if (a == 0b01) { return Vec3(i - 5, 0, 0); }
        if (a == 0b10) { return Vec3(0, i - 5, 0); }
        if (a == 0b11) { return Vec3(0, 0, i - 5); }
        ASSERT(false);
        return Vec3(0, 0, 0);
    }
    static Vec3 from_LLD(uint8_t a, uint8_t i) {
        if (a == 0b01) { return Vec3(i - 15, 0, 0); }
        if (a == 0b10) { return Vec3(0, i - 15, 0); }
        if (a == 0b11) { return Vec3(0, 0, i - 15); }
        ASSERT(false);
        return Vec3(0, 0, 0);
    }
};

struct LDEncoding {
    static LDEncoding from_SLD(Vec3 ld) {
        ASSERT(is_valid_short_ld(ld));
        if (ld.x != 0) {
            return LDEncoding(0b01, ld.x + 5);
        } else if (ld.y != 0) {
            return LDEncoding(0b10, ld.y + 5);
        } else if (ld.z != 0) {
          return LDEncoding(0b11, ld.z + 5);
        }
        return LDEncoding(0xff, 0xff);
    }
    static LDEncoding from_LLD(Vec3 ld) {
        ASSERT(is_valid_long_ld(ld));
        if (ld.x != 0) {
            return LDEncoding(0b01, ld.x + 15);
        } else if (ld.y != 0) {
            return LDEncoding(0b10, ld.y + 15);
        } else if (ld.z != 0) {
            return LDEncoding(0b11, ld.z + 15);
        }
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
    bool operator()(CommandVoid cmd) {
        buffer.push_back(0b00000010 | (nd_encoding(cmd.nd) << 3));
         return true;
    };
    bool operator()(CommandGFill cmd) {
        buffer.push_back(0b00000001 | (nd_encoding(cmd.nd) << 3));
        auto b = NOutputTrace::fd_encoding(cmd.fd);
        buffer.push_back(b[0]);
        buffer.push_back(b[1]);
        buffer.push_back(b[2]);
         return true;
    };
    bool operator()(CommandGVoid cmd) {
        buffer.push_back(0b00000000 | (nd_encoding(cmd.nd) << 3));
        auto b = NOutputTrace::fd_encoding(cmd.fd);
        buffer.push_back(b[0]);
        buffer.push_back(b[1]);
        buffer.push_back(b[2]);
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
    // only for develop & debug
    bool operator()(CommandDebugMoveTo cmd) {
        // no encoding!
        return true;
    };
};

struct EmitCommandJSON : public boost::static_visitor<bool> {
    nlohmann::json& json;

    static nlohmann::json Vec3_to_JSON(Vec3 v) {
        return nlohmann::json::array({v.x, v.y, v.z});
    }
    EmitCommandJSON(nlohmann::json& json_)
        : json(json_) {}

    bool operator()(CommandHalt) { json.push_back({"Halt"}); return true; }
    bool operator()(CommandWait) { json.push_back({"Wait"}); return true; }
    bool operator()(CommandFlip) { json.push_back({"Flip"}); return true; }
    bool operator()(CommandSMove cmd) {
        json.push_back({"SMove", Vec3_to_JSON(cmd.lld)});
        return true;
    };
    bool operator()(CommandLMove cmd) {
        json.push_back({"LMove", Vec3_to_JSON(cmd.sld1), Vec3_to_JSON(cmd.sld2)});
        return true;
    };
    bool operator()(CommandFission cmd) {
        json.push_back({"Fission", Vec3_to_JSON(cmd.nd)});
        return true;
    };
    bool operator()(CommandFill cmd) {
        json.push_back({"Fill", Vec3_to_JSON(cmd.nd)});
        return true;
    };
    bool operator()(CommandVoid cmd) {
        json.push_back({"Void", Vec3_to_JSON(cmd.nd)});
        return true;
    };
    bool operator()(CommandGFill cmd) {
        json.push_back({"GFill", Vec3_to_JSON(cmd.nd), Vec3_to_JSON(cmd.fd)});
        return true;
    };
    bool operator()(CommandGVoid cmd) {
        json.push_back({"GVoid", Vec3_to_JSON(cmd.nd), Vec3_to_JSON(cmd.fd)});
        return true;
    };
    bool operator()(CommandFusionP cmd) {
        json.push_back({"FusionP", Vec3_to_JSON(cmd.nd)});
        return true;
    };
    bool operator()(CommandFusionS cmd) {
        json.push_back({"FusionS", Vec3_to_JSON(cmd.nd)});
        return true;
    };
    // only for develop & debug
    bool operator()(CommandDebugMoveTo cmd) {
        json.push_back({"DebugMoveTo", Vec3_to_JSON(cmd.pos)});
        return true;
    };
};


bool DecodeTrace(Trace& trace, std::vector<uint8_t>& buffer) {
    auto get_next = [&buffer](size_t& i) {
        ++i;
        if (i >= buffer.size()) {
            throw std::runtime_error("unexpected end");
        }
        return buffer[i];
    };
    for (size_t i = 0; i < buffer.size(); ++i) {
        const uint8_t prefixcode = buffer[i];
        if (prefixcode == 0b11111111) {
            trace.push_back(CommandHalt{});
        } else if (prefixcode == 0b11111110) {
            trace.push_back(CommandWait{});
        } else if (prefixcode == 0b11111101) {
            trace.push_back(CommandFlip{});
        } else if ((prefixcode & 0b1111) == 0b0100) {
            const uint8_t nextcode = get_next(i);
            trace.push_back(CommandSMove{
                NOutputTrace::LDDecoding::from_LLD(
                    (prefixcode >> 4) & 0b11,
                    nextcode & 0b11111)});
        } else if ((prefixcode & 0b1111) == 0b1100) {
            const uint8_t nextcode = get_next(i);
            trace.push_back(CommandLMove{
                NOutputTrace::LDDecoding::from_SLD(
                    (prefixcode >> 4) & 0b11,
                    nextcode & 0b1111),
                NOutputTrace::LDDecoding::from_SLD(
                    (prefixcode >> 6) & 0b11,
                    (nextcode >> 4) & 0b1111)});
        } else if ((prefixcode & 0b111) == 0b011) {
            trace.push_back(CommandFill{
                NOutputTrace::nd_decoding(prefixcode >> 3)
                });
        } else if ((prefixcode & 0b111) == 0b010) {
            trace.push_back(CommandVoid{
                NOutputTrace::nd_decoding(prefixcode >> 3)
                });
        } else if ((prefixcode & 0b111) == 0b001) {
            const uint8_t b0 = get_next(i);
            const uint8_t b1 = get_next(i);
            const uint8_t b2 = get_next(i);
            trace.push_back(CommandGFill{
                NOutputTrace::nd_decoding(prefixcode >> 3),
                NOutputTrace::fd_decoding(b0, b1, b2)
                });
        } else if ((prefixcode & 0b111) == 0b000) {
            const uint8_t b0 = get_next(i);
            const uint8_t b1 = get_next(i);
            const uint8_t b2 = get_next(i);
            trace.push_back(CommandGVoid{
                NOutputTrace::nd_decoding(prefixcode >> 3),
                NOutputTrace::fd_decoding(b0, b1, b2)
                });
        } else if ((prefixcode & 0b111) == 0b111) {
            trace.push_back(CommandFusionP{
                NOutputTrace::nd_decoding(prefixcode >> 3)
                });
        } else if ((prefixcode & 0b111) == 0b110) {
            trace.push_back(CommandFusionS{
                NOutputTrace::nd_decoding(prefixcode >> 3)
                });
        } else if ((prefixcode & 0b111) == 0b101) {
            const uint8_t nextcode = get_next(i);
            trace.push_back(CommandFission{
                NOutputTrace::nd_decoding(prefixcode >> 3),
                nextcode
                });
        }
    }
    return true;
}

struct TransposeCommand : public boost::static_visitor<bool> {
    Trace &trace;

    TransposeCommand(Trace &trace_)
        : trace(trace_) {}

    bool operator()(CommandHalt) { trace.push_back(CommandHalt{}); return true; }
    bool operator()(CommandWait) { trace.push_back(CommandWait{}); return true; }
    bool operator()(CommandFlip) { trace.push_back(CommandFlip{}); return true; }
    bool operator()(CommandSMove cmd) {
        cmd.lld = cmd.lld.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandLMove cmd) {
        cmd.sld1 = cmd.sld1.transpose();
        cmd.sld2 = cmd.sld2.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandFission cmd) {
        cmd.nd = cmd.nd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandFill cmd) {
        cmd.nd = cmd.nd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandVoid cmd) {
        cmd.nd = cmd.nd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandGFill cmd) {
        cmd.nd = cmd.nd.transpose();
	cmd.fd = cmd.fd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandGVoid cmd) {
        cmd.nd = cmd.nd.transpose();
	cmd.fd = cmd.fd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandFusionP cmd) {
        cmd.nd = cmd.nd.transpose();
        trace.push_back(cmd);
        return true;
    };
    bool operator()(CommandFusionS cmd) {
        cmd.nd = cmd.nd.transpose();
        trace.push_back(cmd);
        return true;
    };
    // only for develop & debug
    bool operator()(CommandDebugMoveTo cmd) {
        cmd.pos = cmd.pos.transpose();
        trace.push_back(cmd);
        return true;
    };
};


}  // namespace NOutputTrace

bool Trace::output_trace(std::string output_path) {
    std::vector<uint8_t> buf;
    NOutputTrace::EmitCommand visitor(buf);
    for (const auto& command : *this) {
        boost::apply_visitor(visitor, command);
    }

    std::FILE* fp = std::fopen(output_path.c_str(), "wb");
    std::fwrite(buf.data(), 1, buf.size(), fp);
    std::fclose(fp);

    return true;
}

bool Trace::output_trace_json(std::string output_path) {
    nlohmann::json j;
    NOutputTrace::EmitCommandJSON visitor(j);
    for (const auto& command : *this) {
        boost::apply_visitor(visitor, command);
    }

    std::ofstream ofs(output_path);
    ofs << j.dump(4);

    return true;
}


bool Trace::input_trace(std::string input_path) {
    std::FILE* fp = std::fopen(input_path.c_str(), "rb");
    std::fseek(fp, 0, SEEK_END);
    const size_t fsize = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    std::vector<uint8_t> buf(fsize, 0);
    if (fsize != std::fread(buf.data(), 1, buf.size(), fp)) {
        std::printf("error while reading a trace file: %s\n", input_path.c_str());
        return false;
    }
    std::fclose(fp);

    this->clear();
    return NOutputTrace::DecodeTrace(*this, buf);
}


void Trace::print_detailed() {
    std::printf("Trace [size=%ld]\n", size());
    int timestep = 0;
    int active = 1;
    int rest = 1;
    int active_change = 0;
    std::printf("------------[timestep: %7d]\n", timestep);
    for (int i = 0; i < size(); ++i) {
        std::printf("%8d : ", i);
        PrintCommand visitor(std::cout);
        active_change += boost::apply_visitor(visitor, operator[](i));
        std::printf("\n");
        if (--rest == 0 && i + 1 != size()) {
            active += active_change;
            active_change = 0;
            rest = active;
            ++timestep;
            std::printf("------------[timestep: %7d]\n", timestep);
        }
    }
}

Trace Trace::transpose() {
    Trace transposed;
    NOutputTrace::TransposeCommand visitor(transposed);
    for (const auto& command : *this) {
        boost::apply_visitor(visitor, command);
    }
    return transposed;
}

Vec3 Trace::offset() const {
    struct Cursor : public boost::static_visitor<void> {
        Vec3 pos = {0, 0, 0};
        void operator()(const CommandSMove& cmd) { pos += cmd.lld; }
        void operator()(const CommandLMove& cmd) { pos += cmd.sld1 + cmd.sld2; }
        void operator()(...) { }
    } cursor;

    for (const auto& cmd : *this) {
        boost::apply_visitor(cursor, cmd);
    }
    return cursor.pos;
}

void Trace::reduction_smove() {
    if (size() > 1) {
        auto first = this->begin();
        while (std::next(first) != this->end()) {
            const auto second = std::next(first);
            const auto* first_cmd = boost::get<CommandSMove>(&*first);
            const auto* second_cmd = boost::get<CommandSMove>(&*second);
            if (!first_cmd || !second_cmd) {
                ++first;
                continue;
            }
            auto combine = first_cmd->lld + second_cmd->lld;
            if (is_valid_ld(combine) || (combine[0] == 0 && combine[1] == 0 && combine[2] == 0)) {
                first = this->insert(first, CommandSMove{combine});
                ++first;
                first = this->erase(first);
                first = this->erase(first);
                --first;
            } else {
                ++first;
            }
        }
    }
    auto it = this->begin();
    while (it != this->end()) {
        const auto* cmd = boost::get<CommandSMove>(&*it);
        if (cmd && cmd->lld[0] == 0 && cmd->lld[1] == 0 && cmd->lld[2] == 0) {
            it = this->erase(it);
        } else {
            ++it;
        }
    }
    it = this->begin();
    while (it != this->end()) {
        auto* cmd = boost::get<CommandSMove>(&*it);
        if (cmd) {
            auto ld = cmd->lld;
            while (ld[0] > 15 || ld[1] > 15 || ld[2] > 15 || ld[0] < -15 || ld[1] < -15 || ld[2] < -15) {
                auto newlld = getLLD(ld);
                auto newcmd = CommandSMove{newlld};
                it = this->insert(it, newcmd);
                ++it;
            }
            cmd->lld = ld;
        }
        ++it;
    }
}

void Trace::reduction_move() {
    std::vector<Command> commands;
    for (const auto& original_command : *this) {
        ASSERT(!boost::get<CommandFission>(&original_command));
        ASSERT(!boost::get<CommandFusionP>(&original_command));
        ASSERT(!boost::get<CommandFusionS>(&original_command));
        if (const auto* command = boost::get<CommandSMove>(&original_command)) {
            commands.insert(commands.end(), command->lld.manhattan_length(), CommandSMove{ command->lld.unit_vector() });
        }
        else if (const auto* command = boost::get<CommandLMove>(&original_command)) {
            commands.insert(commands.end(), command->sld1.manhattan_length(), CommandSMove{ command->sld1.unit_vector() });
            commands.insert(commands.end(), command->sld2.manhattan_length(), CommandSMove{ command->sld2.unit_vector() });
        }
        else {
            commands.push_back(original_command);
        }
    }

    clear();

    enum State {
        Normal,
        Move1,
        Move2,
    } state = Normal;

    Vec3 move1;
    int length1 = 0;
    Vec3 move2;
    int length2 = 0;
    for (const auto& original_command : commands) {
        if (const auto* command = boost::get<CommandSMove>(&original_command)) {
            switch (state) {
            case Normal:
                state = Move1;
                move1 = command->lld;
                length1 = 1;
                break;
            case Move1:
                if (command->lld == move1) {
                    // Same direction.
                    if (length1 == 15) {
                        // Push CommandSMove and start the next CommandSMove.
                        push_back(CommandSMove{ move1 * length1 });
                        length1 = 1;
                    }
                    else {
                        // Continue CommandSMove.
                        ++length1;
                    }
                } else {
                    // Different direction.
                    if (length1 > 5) {
                        // Push CommandSMove and start the next CommandSMove.
                        push_back(CommandSMove{ move1 * length1 });
                        move1 = command->lld;
                        length1 = 1;
                    }
                    else {
                        // Start the second direction.
                        move2 = command->lld;
                        length2 = 1;
                        state = Move2;
                    }
                }
                break;
            case Move2:
                if (command->lld == move2) {
                    // Same direction.
                    if (length2 == 5) {
                        // Push CommandLMove and start the next CommandSMove.
                        push_back(CommandLMove{ move1 * length1, move2 * length2 });
                        state = Move1;
                        move1 = command->lld;
                        length1 = 1;
                        move2 = Vec3();
                        length2 = 0;
                    }
                    else {
                        // Continue CommandLMove.
                        ++length2;
                    }
                }
                else {
                    // Different direction.
                    // Push CommandLMove and start the next CommandSMove.
                    push_back(CommandLMove{ move1 * length1, move2 * length2 });
                    state = Move1;
                    move1 = command->lld;
                    length1 = 1;
                    move2 = Vec3();
                    length2 = 0;
                }
                break;
            default:
                ASSERT(false);
            }
        }
        else {
            switch (state) {
            case Normal:
                break;
            case Move1:
                push_back(CommandSMove{ move1 * length1 });
                move1 = Vec3();
                length1 = 0;
                state = Normal;
                break;
            case Move2:
                push_back(CommandLMove{ move1 * length1, move2 * length2 });
                move1 = Vec3();
                length1 = 0;
                move2 = Vec3();
                length2 = 0;
                state = Normal;
                break;
            default:
                ASSERT(false);
            }
            push_back(original_command);
        }
    }

    switch (state) {
    case Normal:
        break;
    case Move1:
        push_back(CommandSMove{ move1 * length1 });
        move1 = Vec3();
        length1 = 0;
        state = Normal;
        break;
    case Move2:
        push_back(CommandLMove{ move1 * length1, move2 * length2 });
        move1 = Vec3();
        length1 = 0;
        move2 = Vec3();
        length2 = 0;
        state = Normal;
        break;
    default:
        ASSERT(false);
    }
}

// vim: set si et sw=4 ts=4:
