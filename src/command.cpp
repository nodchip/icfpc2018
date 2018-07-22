#include "command.h"
#include <sstream>

std::string PrintCommand::Vec3_to_string(Vec3 p) {
    std::ostringstream oss;
    oss << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return oss.str();
}
int PrintCommand::operator()(CommandHalt) { std::printf("<Halt>"); return -1; }
int PrintCommand::operator()(CommandWait) { std::printf("<Wait>"); return 0; }
int PrintCommand::operator()(CommandFlip) { std::printf("<Flip>"); return 0; }
int PrintCommand::operator()(CommandSMove cmd) {
    std::printf("<SMove %s>", Vec3_to_string(cmd.lld).c_str());
    return 0;
};
int PrintCommand::operator()(CommandLMove cmd) {
    std::printf("<LMove %s %s>", Vec3_to_string(cmd.sld1).c_str(), Vec3_to_string(cmd.sld2).c_str());
    return 0;
};
int PrintCommand::operator()(CommandFission cmd) {
    std::printf("<Fission %s>", Vec3_to_string(cmd.nd).c_str());
    return 1;
};
int PrintCommand::operator()(CommandFill cmd) {
    std::printf("<Fill %s>", Vec3_to_string(cmd.nd).c_str());
    return 0;
};
int PrintCommand::operator()(CommandVoid cmd) {
    std::printf("<Void %s>", Vec3_to_string(cmd.nd).c_str());
    return 0;
};
int PrintCommand::operator()(CommandGFill cmd) {
    std::printf("<GFill %s %s>", Vec3_to_string(cmd.nd).c_str(), Vec3_to_string(cmd.fd).c_str());
    return 0;
};
int PrintCommand::operator()(CommandGVoid cmd) {
    std::printf("<GVoid %s %s>", Vec3_to_string(cmd.nd).c_str(), Vec3_to_string(cmd.fd).c_str());
    return 0;
};
int PrintCommand::operator()(CommandFusionP cmd) {
    std::printf("<FusionP %s>", Vec3_to_string(cmd.nd).c_str());
    return -1;
};
int PrintCommand::operator()(CommandFusionS cmd) {
    std::printf("<FusionS %s>", Vec3_to_string(cmd.nd).c_str());
    return 0;
};
// only for develop & debug
int PrintCommand::operator()(CommandDebugMoveTo cmd) {
    std::printf("<DebugMoveTo %s>", Vec3_to_string(cmd.pos).c_str());
    return 0;
    };
