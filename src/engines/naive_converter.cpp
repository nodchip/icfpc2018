#include <algorithm>
#include "naive_converter.h"
#include "log.h"

namespace NaiveConverter {

// Assembly solver -> Disassembly solver
SolverType reverse(const SolverType& assembly_solver) {
    return [&](ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) -> Trace {
        ASSERT_RETURN(problem_type == ProblemType::Disassembly, Trace());
        const auto trace = assembly_solver(ProblemType::Assembly, Matrix{}, src_matrix);

        Trace reversed_trace;
        struct InvertCommand : public boost::static_visitor<void> {
            int num_active_nanobots_;
            std::size_t previous_size_;
            Trace& trace_;
            InvertCommand(Trace& trace) :
                num_active_nanobots_(1), previous_size_(0), trace_(trace) {}
            void FinalizeTimeStep() {
                ASSERT(trace_.size() <= previous_size_ + num_active_nanobots_);
                if (trace_.size() == previous_size_ + num_active_nanobots_) {
                    std::reverse(trace_.end() - num_active_nanobots_, trace_.end());
                    previous_size_ = trace_.size();
                }
            }
            void operator()(CommandHalt) {
                // do nothing
            }
            void operator()(CommandWait) {
                trace_.push_back(CommandWait{});
                FinalizeTimeStep();
            }
            void operator()(CommandFlip) {
                trace_.push_back(CommandFlip{});
                FinalizeTimeStep();
            }
            void operator()(CommandSMove cmd) {
                trace_.push_back(CommandSMove{-cmd.lld});
                FinalizeTimeStep();
            }
            void operator()(CommandLMove cmd) {
                trace_.push_back(CommandLMove{-cmd.sld2, -cmd.sld1});
                FinalizeTimeStep();
            }
            void operator()(CommandFission cmd) {
                trace_.push_back(CommandFusionP{cmd.nd});
                trace_.push_back(CommandFusionS{-cmd.nd});
                ++num_active_nanobots_;
                FinalizeTimeStep();
            }
            void operator()(CommandFill cmd) {
                trace_.push_back(CommandVoid{cmd.nd});
                FinalizeTimeStep();
            }
            void operator()(CommandVoid cmd) {
                trace_.push_back(CommandFill{cmd.nd});
                FinalizeTimeStep();
            }
            void operator()(CommandGFill cmd) {
                trace_.push_back(CommandGVoid{cmd.nd, cmd.fd});
                FinalizeTimeStep();
            }
            void operator()(CommandGVoid cmd) {
                trace_.push_back(CommandGVoid{cmd.nd, cmd.fd});
                FinalizeTimeStep();
            }
            void operator()(CommandFusionP cmd) {
                trace_.push_back(CommandFission{
                        cmd.nd, k_MaxNumberOfBots - num_active_nanobots_});
                --num_active_nanobots_;
                FinalizeTimeStep();
            }
            void operator()(CommandFusionS cmd) {
                // do nothing
            }
            void operator()(CommandDebugMoveTo cmd) {
                ASSERT(false);
            }
        } visitor(reversed_trace);

        for (const auto& cmd : trace) {
            boost::apply_visitor(visitor, cmd);
        }
        std::reverse(reversed_trace.begin(), reversed_trace.end());
        reversed_trace.push_back(CommandHalt{});

        return reversed_trace;
    };
}

// Disassembly and Assembly solvers -> Reassembly solver
SolverType concatenate(const SolverType& disassembly_solver, const SolverType& assembly_solver) {
    return [&](ProblemType problem_type, const Matrix& src_matrix, const Matrix& tgt_matrix) -> Trace {
        ASSERT_RETURN(problem_type == ProblemType::Reassembly, Trace());
        auto trace = disassembly_solver(ProblemType::Disassembly, src_matrix, Matrix{});
        ASSERT_RETURN(!trace.empty(), Trace());
        ASSERT_RETURN(boost::get<CommandHalt>(&trace.back()), Trace());
        trace.pop_back();

        const auto assembly_trace = assembly_solver(ProblemType::Assembly, Matrix{}, tgt_matrix);
        trace.insert(trace.end(), assembly_trace.begin(), assembly_trace.end());

        return trace;
    };
}

}  // namespace NaiveConverter

// vim: set si et sw=4 ts=4:
