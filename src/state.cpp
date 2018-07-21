#include "state.h"

#include "nmms.h"
#include "matrix.h"
#include "system.h"

State::State(const Matrix& m)
  : problem(m), sys(m.R) {}

int State::simulate(const Trace& t) {
    sys.trace = t;
    while (!sys.trace.empty()) {
        if (proceed_timestep(sys)) {
            break;
        }
    }
    sys.print_detailed();

    int exit_code = 0;
    bool is_successful = is_finished(sys, problem);
    if (is_successful) {
        std::cout << "Success." << std::endl;
    } else {
        std::cout << "Final state is not valid." << std::endl;
        exit_code = 4;
    }

    return exit_code;
}
