#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <string>
// 3rd
#include <nlohmann/json.hpp>
// project
#include "engine.h"
#include "matrix.h"
#include "state.h"
#include "trace.h"
#include "log.h"

const Vec3 unitX(1, 0, 0);
const Vec3 unity(0, 1, 0);
const Vec3 unitZ(0, 0, 1);

State sandbox_test() {
    const int R = 50;
    const int N = 10;
    Matrix msrc(R), mtgt(R);
    State state(msrc, mtgt);
    Trace& trace = state.system.trace;

    // linear dense distribution.
    int active = 1;
    while (active < N) {
        trace.push_back(CommandSMove{unitX});
        for (int i = 0; i < active - 1; ++i) {
            trace.push_back(CommandWait{});
        }
        //
        trace.push_back(CommandFission{-unitX, 1});
        for (int i = 0; i < active - 1; ++i) {
            trace.push_back(CommandWait{});
        }
        ++active;
    }

    trace.print_detailed();
    
    return state;
}

State sandbox_stepbystep() {
    const int R = 50;
    const int N = 10;
    Matrix msrc(R), mtgt(R);
    State state(msrc, mtgt);
    System& system = state.system;
    system.set_verbose(true);

    // linear dense distribution.
    for (int active = system.bots.size(); active < N; ++active) {
        system.stage(system.bots[0], CommandSMove{unitX});
        for (int i = 1; i < system.bots.size(); ++i) {
            system.stage(system.bots[i], CommandWait{});
        }
        system.commit_commands();
        system.print_detailed();

        system.stage(system.bots[0], CommandFission{-unitX, 1});
        for (int i = 1; i < system.bots.size(); ++i) {
            system.stage(system.bots[i], CommandWait{});
        }
        system.commit_commands();
        system.print_detailed();
    }

    system.trace.print_detailed();
    
    return state;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        return 1;
    }

    std::string cmd = argv[1];
    State state = [&] {
        if (cmd == "test") { return sandbox_test(); }
        if (cmd == "stepbystep") { return sandbox_stepbystep(); }
        // add your function here..

        return State { {}, {} };
    }();

    state.simulate(state.system.trace);
    state.system.print_detailed();

    return 0;
}
// vim: set si et sw=4 ts=4:
