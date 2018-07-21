#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <string>
// 3rd
#include <nlohmann/json.hpp>
// project
#include "engine.h"
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
    // ("help", "produce help message");
    // ("model", po::value<std::string>(), "model.mdl");
    // ("trace-output", po::value<std::string>(), "trace.nbt");
    // ("engine", po::value<std::string>()->default_value("default"), "engine");
    // ("info", po::value<std::string>()->default_value("info"), "info path");

    if (options.count("help")) {
        std::cout << "produce help message" << "\n";
        return 1;
    }

    // find engine(optional)
    if (RegisterEngine::Engines().empty()) {
        std::cout << "no engines are registered." << std::endl;
    }

    EngineFunc engine;
    std::string engine_name;
    if (options.count("engine") == 0 || options["engine"] == "default") {
        auto it = RegisterEngine::Engines().begin();
        engine = it->second;
        engine_name = it->first;
    } else {
        auto it = RegisterEngine::Engines().find(options["engine"]);
        if (it != RegisterEngine::Engines().end()) {
            it = RegisterEngine::Engines().begin();
        } else {
            std::cout << "Failed to find engine: "
                      << options["engine"] << std::endl;
            return 2;
        }
        engine = it->second;
        engine_name = it->first;
    }

    // load
    if (!options.count("model")) {
        std::cout << "--model required." << std::endl;
        return 3;
    }
    auto model_path = options["model"];
    Matrix m = load_model(model_path);
    if (!m) {
        std::cout << "Failed to open model file: " << model_path << std::endl;
        return 3;
    }

    // init
    System sys;
    sys.start(m.R);

    // sys is not changed in the solver.
    auto trace = engine(sys, m);

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

    if (options.count("trace-output")) {
        // dump the result.
        auto dump_model_path = options["trace-output"] + ".mdl";
        auto dump_trace_path = options["trace-output"];
        dump_model(dump_model_path, sys.matrix);

        // trace.
        trace.output_trace(dump_trace_path);
    }

    if (options.count("info")) {
        nlohmann::json j = {
            {"energy", sys.energy},
            {"consumed_commands", sys.consumed_commands},
            {"successful", is_successful},
            {"engine_name", engine_name},
        };
        std::ofstream ofs(options["info"]);
        ofs << std::setw(4) << j;
    }

    return exit_code;
}
// vim: set si et sw=4 ts=4:
