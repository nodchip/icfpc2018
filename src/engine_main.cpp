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
#include "state.h"
#include "debug_message.h"

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
    // ("src-model", po::value<std::string>(), "model.mdl");
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

    // load (source) model
    Matrix src_model;
    if (options.count("src-model")) {
        auto model_path = options["src-model"];
        src_model = Matrix(model_path);
        if (!src_model.is_valid_matrix()) {
            std::cout << "Failed to open model file: " << model_path << std::endl;
            return 3;
        }
    }

    // load (target) model
    Matrix tgt_model;
    if (options.count("model")) {
        auto model_path = options["model"];
        tgt_model = Matrix(model_path);
        if (!tgt_model.is_valid_matrix()) {
            std::cout << "Failed to open model file: " << model_path << std::endl;
            return 3;
        }
    }

    auto problem_type = determine_problem_type_and_prepare_matrices(src_model, tgt_model);
    ASSERT_ERROR_RETURN(problem_type != ProblemType::Invalid, 3);

    auto trace = engine(problem_type, src_model, tgt_model);
    std::cout << "generated trace: " << trace.size() << std::endl;

    State state(src_model, tgt_model);
    std::cout << "simulation prepare." << std::endl;
    int exit_code = state.simulate(trace);
    std::cout << "simulation done." << std::endl;

    if (options.count("trace-output")) {
        // dump the result.
        auto dump_model_path = options["trace-output"] + ".mdl";
        auto dump_trace_path = options["trace-output"];
        state.system.matrix.dump(dump_model_path);

        // trace.
        trace.output_trace(dump_trace_path);
    }

    if (options.count("info")) {
        nlohmann::json j = {
            {"energy", state.system.energy},  // should be state.energy
            {"consumed_commands", state.system.consumed_commands},
            {"successful", exit_code == 0},
            {"engine_name", engine_name},
        };
        std::ofstream ofs(options["info"]);
        ofs << std::setw(4) << j;
    }

    return exit_code;
}
// vim: set si et sw=4 ts=4:
