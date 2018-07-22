#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <string>
// 3rd
#include <nlohmann/json.hpp>
// project
#include "traceutil.h"
#include "engine.h"
#include "matrix.h"
#include "state.h"
#include "trace.h"
#include "log.h"

const Vec3 unitX(1, 0, 0);
const Vec3 unitY(0, 1, 0);
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
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();

        system.stage(system.bots[0], CommandFission{-unitX, 1});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();
    }

    system.trace.print_detailed();
    
    return state;
}

State sandbox_Nx2x2() {
    const int R = 50;
    const int N = 40;
    Matrix msrc(R), mtgt(R);
    State state(msrc, mtgt);
    System& system = state.system;
    system.set_verbose(true);

    // 2x2 dense distribution.
    while (system.bots.size() < N) {
        system.stage(system.bots[0], CommandFission{unitY, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();

        system.stage(system.bots[0], CommandFission{unitY + unitZ, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();

        system.stage(system.bots[0], CommandFission{unitZ, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();

        if (system.bots.size() == N) {
            break;
        }

        system.stage(system.bots[0], CommandSMove{unitX});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();

        system.stage(system.bots[0], CommandFission{-unitX, 0});
        system.stage_all_unstaged(CommandWait{});
        system.commit_commands();
        system.print_detailed();
    }

    system.trace.print_detailed();
    
    return state;
}

State sandbox_expand_collapse() {
    const int R = 50;
    const int N = 8;
    Matrix msrc(R), mtgt(R);
    State state(msrc, mtgt);
    System& system = state.system;

    Trace trace;
    { // Nx2x2
        std::vector<Vec3> id_to_pos;
        ASSERT(NTraceUtil::fission_x_2by2_linear_positions(
            system.bots[0].pos, N, R, id_to_pos, trace));
        ASSERT(id_to_pos.size() == N);

        for (auto p : id_to_pos) {
            LOG() << "pos " << p << std::endl;
        }
    }
    if (true) { // tekitou move.
        for (int j = 0; j < 4; ++j) {
            for (int i = 0; i < N; ++i) {
                trace.push_back(CommandSMove{unitX});
            }
            for (int i = 0; i < N; ++i) {
                trace.push_back(CommandSMove{unitY});
            }
            for (int i = 0; i < N; ++i) {
                trace.push_back(CommandSMove{unitZ});
            }
        }
    }
    state.append_simulate_partial(trace);
    trace.clear();
    { // go home.
        auto linear_positions = NTraceUtil::create_x_linear_positions(system.matrix, N);
        auto current_bot_positions = NTraceUtil::bot_positions(system);
        NTraceUtil::sort_by_bid(linear_positions, system.bots);
        NTraceUtil::sort_by_bid(current_bot_positions, system.bots);

        state.system.set_verbose(true);
        LOG() << "*** MERGE ***\n";
        LOG() << "bots : " << current_bot_positions.size() << std::endl;
        for (auto p : current_bot_positions) LOG() << p << std::endl;
        LOG() << "pos : " << linear_positions.size() << std::endl;
        for (auto p : linear_positions) LOG() << p << std::endl;

        LOG() << "*** evacuate\n";
        { // first, evacuate each.
            std::vector<Trace> traces(current_bot_positions.size());
            for (size_t i = 0; i < current_bot_positions.size(); ++i) {
                LOG() << "evacuate " << i << " " << current_bot_positions[i] << "\n";
                NTraceUtil::digging_evacuate(system.matrix,
                    current_bot_positions[i] /* updated */, traces[i]);
                LOG() << "     -> " << i << " " << current_bot_positions[i] << "\n";
            }
            NTraceUtil::merge_traces(traces, trace);
            state.append_simulate_partial(trace);
            trace.print_detailed();
            trace.clear();
        }
        LOG() << "*** move\n";
        { // move to target positions.
            NTraceUtil::move_evacuated_multibots(system.matrix,
                current_bot_positions,
                linear_positions, trace);
            state.append_simulate_partial(trace);
            trace.clear();
        }
        LOG() << "*** fusion\n";
        ASSERT(linear_positions.size() == N);
        ASSERT(NTraceUtil::fusion_x_linear_positions_to_first_pos(linear_positions, trace));
        state.append_simulate_partial(trace);
        trace.clear();

        // Halt
        trace.push_back(CommandHalt{});
        state.append_simulate_partial(trace);
        trace.clear();

        LOG() << "*** MERGE ***\n";
    }

    trace = system.trace;

    LOG() << "\n===========================simulation=\n";
    {
        Matrix msrc(R), mtgt(R);
        State state(msrc, mtgt);
        state.system.set_verbose(true);
        state.simulate(trace);
        LOG() << "OK? = " << state.is_finished() << std::endl;
    }
    
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
        if (cmd == "Nx2x2") { return sandbox_Nx2x2(); }
        if (cmd == "expand_collapse") { return sandbox_expand_collapse(); }
        // add your function here..

        return State { {}, {} };
    }();

    state.simulate(state.system.trace);
    state.system.print_detailed();

    return 0;
}
// vim: set si et sw=4 ts=4:
