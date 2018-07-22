#include "command.h"
#include <sstream>

int PrintCommand::operator()(CommandHalt) { os << "<Halt>"; return -1; }
int PrintCommand::operator()(CommandWait) { os << "<Wait>"; return 0; }
int PrintCommand::operator()(CommandFlip) { os << "<Flip>"; return 0; }
int PrintCommand::operator()(CommandSMove cmd) {
    os << "<SMove " << cmd.lld << ">";
    return 0;
};
int PrintCommand::operator()(CommandLMove cmd) {
    os << "<LMove " << cmd.sld1 << " " << cmd.sld2 << ">";
    return 0;
};
int PrintCommand::operator()(CommandFission cmd) {
    os << "<Fission " << cmd.nd << ">";
    return 1;
};
int PrintCommand::operator()(CommandFill cmd) {
    os << "<Fill " << cmd.nd << ">";
    return 0;
};
int PrintCommand::operator()(CommandVoid cmd) {
    os << "<Void " << cmd.nd << ">";
    return 0;
};
int PrintCommand::operator()(CommandGFill cmd) {
    os << "<GFill " << cmd.nd << " " << cmd.fd << ">";
    return 0;
};
int PrintCommand::operator()(CommandGVoid cmd) {
    os << "<GVoid " << cmd.nd << " " << cmd.fd << ">";
    return 0;
};
int PrintCommand::operator()(CommandFusionP cmd) {
    os << "<FusionP " << cmd.nd << ">";
    return -1;
};
int PrintCommand::operator()(CommandFusionS cmd) {
    os << "<FusionS " << cmd.nd << ">";
    return 0;
};
// only for develop & debug
int PrintCommand::operator()(CommandDebugMoveTo cmd) {
    os << "<DebugMoveTo " << cmd.pos << ">";
    return 0;
};
