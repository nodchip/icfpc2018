#include "state.h"

#include "matrix.h"
#include "system.h"
#include "log.h"

State::State(const Matrix& src_matrix, const Matrix& tgt_matrix)
  : src_problem(src_matrix)
  , tgt_problem(tgt_matrix)
  , system(src_matrix.R) {
    ASSERT(src_matrix.is_valid_matrix());
    ASSERT(tgt_matrix.is_valid_matrix());
    ASSERT_EQ(src_matrix.R, tgt_matrix.R);
    system.matrix = src_matrix;
}

int State::simulate(const Trace& t) {
    try {
        system.trace = t;
        while (!system.is_eof()) {
            if (system.proceed_timestep()) {
                break;
            }
        }
        system.print_detailed();
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        std::cout << "dump the consumed trace." << std::endl;
        Trace consumed;
        auto it = t.begin();
        std::advance(it, t.size() - system.trace.size());
        std::copy(t.begin(), it, std::back_inserter(consumed));
        consumed.output_trace("exception.nbt");
        consumed.output_trace_json("exception.nbt.json");
        throw;
    }

    int exit_code = 0;
    bool is_successful = is_finished();
    if (is_successful) {
        std::cout << "Success." << std::endl;
    } else {
        std::cout << "Final state is not valid." << std::endl;
        exit_code = 4;
    }

    return exit_code;
}

bool State::is_finished() const {
    if (system.harmonics_high) {
        LOG() << "harmonics are still high.\n";
        return false;
    }
    if (!system.bots.empty()) {
        LOG() << "bots are remaining.\n";
        return false;
    }
    // if (!system.is_eof())
    //     return false;
    if (system.matrix != tgt_problem) {
        LOG() << "matrix is different from the target";
        print_difference(system.matrix, tgt_problem);
        return false;
    }

    return true;
}

ProblemType determine_problem_type_and_prepare_matrices(Matrix& src_matrix, Matrix& tgt_matrix) {
    ASSERT_RETURN(src_matrix.is_valid_matrix() || tgt_matrix.is_valid_matrix(), ProblemType::Invalid);

    if (src_matrix.is_valid_matrix() && tgt_matrix.is_valid_matrix()) {
        printf("Problem: Reassembly\n");
        return ProblemType::Reassembly;
    }

    if (src_matrix.is_valid_matrix() && !tgt_matrix.is_valid_matrix()) {
        printf("Problem: Disassembly\n");
        tgt_matrix = Matrix(src_matrix.R, Void);
        return ProblemType::Disassembly;
    }

    if (!src_matrix.is_valid_matrix() && tgt_matrix.is_valid_matrix()) {
        printf("Problem: Assembly\n");
        src_matrix = Matrix(tgt_matrix.R, Void);
        return ProblemType::Assembly;
    }

    return ProblemType::Invalid;
}
