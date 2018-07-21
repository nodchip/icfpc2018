#include "trace.h"

#include <vector>
#include "debug_message.h"

namespace NOutputTrace {

uint8_t nd_encoding(Vec3 nd) {
    ASSERT_ERROR(is_valid_nd(nd));
    return (nd.x + 1) * 9 + (nd.y + 1) * 3 + (nd.z + 1);
}
Vec3 nd_decoding(uint8_t b) {
    int z = b % 3 - 1;
    int y = b / 3 % 3 - 1;
    int x = b / 9 - 1;
    return Vec3(x, y, z);
}

struct LDDecoding {
    static Vec3 from_SLD(uint8_t a, uint8_t i) {
        if (a == 0b01) { return Vec3(i - 5, 0, 0); }
        if (a == 0b10) { return Vec3(0, i - 5, 0); }
        if (a == 0b11) { return Vec3(0, 0, i - 5); }
        ASSERT_ERROR(false);
        return Vec3(0, 0, 0);
    }
    static Vec3 from_LLD(uint8_t a, uint8_t i) {
        if (a == 0b01) { return Vec3(i - 15, 0, 0); }
        if (a == 0b10) { return Vec3(0, i - 15, 0); }
        if (a == 0b11) { return Vec3(0, 0, i - 15); }
        ASSERT_ERROR(false);
        return Vec3(0, 0, 0);
    }
};

struct LDEncoding {
    static LDEncoding from_SLD(Vec3 ld) {
        ASSERT_ERROR(is_valid_short_ld(ld));
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
        ASSERT_ERROR(is_valid_long_ld(ld));
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
    bool operator()(CommandFusionP cmd) {
        buffer.push_back(0b00000111 | (nd_encoding(cmd.nd) << 3));
        return true;
    };
    bool operator()(CommandFusionS cmd) {
        buffer.push_back(0b00000110 | (nd_encoding(cmd.nd) << 3));
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

// vim: set si et sw=4 ts=4:
