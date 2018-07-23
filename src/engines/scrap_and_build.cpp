#include "engines/naive_converter.h"
#include "engine.h"
#include "nmms.h"
#include "system.h"
#include "state.h"
#include "log.h"

namespace {

Trace solver(ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) {
    ASSERT_RETURN(problem_type == ProblemType::Reassembly, Trace());

    int64_t best_disassembly_energy = std::numeric_limits<int64_t>::max();
    int64_t best_assembly_energy = std::numeric_limits<int64_t>::max();
    EngineFunc best_disassembly_engine;
    EngineFunc best_assembly_engine;
    for (const auto& name_and_engine : RegisterEngine::Engines()) {
        std::cout << "debug: engine name: " << name_and_engine.first << std::endl;
        const auto& engine = name_and_engine.second;

        do {  // disassembly
            const auto trace = engine(ProblemType::Disassembly, src_matrix, Matrix{});
            if (trace.empty()) break;
            std::cout << "trying disassembly with " << name_and_engine.first << std::endl;
            State state(src_matrix, Matrix{tgt_matrix.R});
            state.simulate(trace);
            if (!state.is_finished()) break;
            if (state.system.energy < best_disassembly_energy) {
                best_disassembly_energy = state.system.energy;
                best_disassembly_engine = engine;
            }
        } while (false);

        do {  // assembly
            const auto trace = engine(ProblemType::Assembly, Matrix{}, tgt_matrix);
            if (trace.empty()) break;
            std::cout << "trying assembly with " << name_and_engine.first << std::endl;
            State state(Matrix{src_matrix.R}, tgt_matrix);
            state.simulate(trace);
            if (!state.is_finished()) break;
            if (state.system.energy < best_assembly_energy) {
                best_assembly_energy = state.system.energy;
                best_assembly_engine = engine;
            }
        } while (false);
    }
    ASSERT_RETURN(best_disassembly_engine, Trace());
    ASSERT_RETURN(best_assembly_engine, Trace());
    return NaiveConverter::concatenate(best_disassembly_engine, best_assembly_engine)(problem_type, src_matrix, tgt_matrix);
}

}  // namespace

REGISTER_ENGINE(_scrap_and_build, solver);
