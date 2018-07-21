// ./evaluate --model <model input path> --trace <trace input path> --info <info output path>
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
#include "nmms.h"

using Options = std::map<std::string, std::string>;

// Simply parse command  line options. not accept "--foo=bar"
Options ParseCommand(int argc, char** argv) {
  Options options;

  std::string key, value;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg[0] != '-') {
      // value
      // ASSERT(key != "");
      options[key] = arg;
      key = "";
      continue;
    }

    int c = arg.find_first_not_of("-");
    key = arg.substr(c);
    value = "";
  }

  return options;
}

int main(int argc, char** argv) {
    Options options = ParseCommand(argc, argv);

    if (options.count("help")) {
        std::cout << "produce help message" << "\n";
        return 1;
    }

    // load
    if (!options.count("model")) {
        std::cout << "--model required." << std::endl;
        return 3;
    }
    auto model_path = options["model"];
    Matrix m(model_path);
    if (!m) {
        std::cout << "Failed to open model file: " << model_path << std::endl;
        return 3;
    }

    // load trace
    if (!options.count("trace")) {
        std::cout << "--trace required." << std::endl;
        return 4;
    }
    auto trace_path = options["trace"];
    Trace trace;
    if (!trace.input_trace(trace_path)) {
        std::cout << "Failed to open trace file: " << trace_path << std::endl;
        return 4;
    }

    State state(m);
    int exit_code = state.simulate(trace);
    state.system.print_detailed();

    if (options.count("info")) {
        nlohmann::json j = {
            {"energy", state.system.energy},  // should be state.energy
            {"consumed_commands", state.system.consumed_commands},
            {"successful", exit_code == 0},
            {"engine_name", "unknown"},
        };
        std::ofstream ofs(options["info"]);
        ofs << std::setw(4) << j;
    }

    return exit_code;
}
// vim: set si et sw=4 ts=4:
