#include "state.h"

#include "matrix.h"
#include "system.h"

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
    if (system.harmonics_high)
        return false;
    if (!system.bots.empty())
        return false;
    // if (!system.trace.empty())
    //     return false;
    if (system.matrix != problem)
        return false;

    return true;
}
