#include "state.h"

#include "matrix.h"
#include "system.h"
#include "debug_message.h"

State::State(const Matrix& src_matrix, const Matrix& tgt_matrix)
  : src_problem(src_matrix)
  , tgt_problem(tgt_matrix)
  , system(src_matrix.R) {
    ASSERT_ERROR(src_matrix.is_valid_matrix());
    ASSERT_ERROR(tgt_matrix.is_valid_matrix());
    ASSERT_ERROR(src_matrix.R == tgt_matrix.R);
}

int State::simulate(const Trace& t) {
    system.trace = t;
    while (!system.trace.empty()) {
        if (system.proceed_timestep()) {
            break;
        }
    }
    system.print_detailed();

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
        LOG_ERROR("harmonics are still high.");
        return false;
    }
    if (!system.bots.empty()) {
        LOG_ERROR("bots are remaining.");
        return false;
    }
    // if (!system.trace.empty())
    //     return false;
    if (system.matrix != tgt_problem) {
        LOG_ERROR("matrix is different from the target");
        return false;
    }

    return true;
}

ProblemType determine_problem_type_and_prepare_matrices(Matrix& src_matrix, Matrix& tgt_matrix) {
    ASSERT_ERROR_RETURN(src_matrix.is_valid_matrix() || tgt_matrix.is_valid_matrix(), ProblemType::Invalid);

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
