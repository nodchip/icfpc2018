#include <iostream>
#include <fstream>
// 3rd
#include <boost/program_options.hpp>
#include <nlohmann/json.hpp>
// project
#include "nmms.h"
#include "engine.h"

std::map<std::string, EngineFunc> RegisterEngine::engines = {};

int main(int argc, char** argv) {
    namespace po = boost::program_options;

    po::options_description desc("Engine");
    desc.add_options()
        ("help", "produce help message")
        ("model", po::value<std::string>(), "model.mdl")
        ("trace-output", po::value<std::string>(), "trace.nbt")
        ("engine", po::value<std::string>()->default_value("default"), "engine")
        ("info", po::value<std::string>()->default_value("info"), "info path")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);    

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    // find engine(optional)
    if (RegisterEngine::engines.empty()) {
        std::cout << "no engines are registered." << std::endl;
    }

    EngineFunc engine;
    if (vm["engine"].as<std::string>() == "default") {
        engine = RegisterEngine::engines.begin()->second;
    } else {
        auto it = RegisterEngine::engines.find(vm["engine"].as<std::string>());
        if (it != RegisterEngine::engines.end()) {
            it = RegisterEngine::engines.begin();
        } else {
            std::cout << "Failed to find engine: " << vm["engine"].as<std::string>() << std::endl;
            return 2;
        }
        engine = it->second;
    }

    // load
    if (!vm.count("model")) {
        std::cout << "--model required." << std::endl;
        return 3;
    }
    auto model_path = vm["model"].as<std::string>();
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

    if (vm.count("trace-output")) {
        // dump the result.
        auto dump_model_path = vm["trace-output"].as<std::string>() + ".mdl";
        auto dump_trace_path = vm["trace-output"].as<std::string>();
        dump_model(dump_model_path, sys.matrix);

        // trace.
        output_trace(dump_trace_path, trace);
    }

    if (vm.count("info")) {
        nlohmann::json j = {
            {"energy", sys.energy},
            {"consumed_commands", sys.consumed_commands},
            {"successful", is_successful},
        };
        std::ofstream ofs(vm["info"].as<std::string>());
        ofs << std::setw(4) << j;
    }

    return exit_code;
}
// vim: set si et sw=4 ts=4:
