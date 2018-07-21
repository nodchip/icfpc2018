#include "state.h"

#include "matrix.h"
#include "system.h"
#include "debug_message.h"

State::State(const Matrix& m)
  : problem(m), system(m.R) {}

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
    if (system.matrix != problem) {
        LOG_ERROR("matrix is different from the target");
        return false;
    }

    return true;
}
