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

    // init
    System sys(m.R);

    // simulate the plan.
    sys.trace = trace;
    while (!sys.trace.empty()) {
        if (proceed_timestep(sys)) {
            break;
        }
    }
    sys.print_detailed();

    int exit_code = 0;
    bool is_successful = is_finished(sys, m);
    if (is_successful) {
        std::cout << "Success." << std::endl;
    } else {
        std::cout << "Final state is not valid." << std::endl;
        exit_code = 4;
    }

    if (options.count("info")) {
        nlohmann::json j = {
            {"energy", sys.energy},
            {"consumed_commands", sys.consumed_commands},
            {"successful", is_successful},
            {"engine_name", "unknown"},
        };
        std::ofstream ofs(options["info"]);
        ofs << std::setw(4) << j;
    }

    return exit_code;
}
// vim: set si et sw=4 ts=4:
