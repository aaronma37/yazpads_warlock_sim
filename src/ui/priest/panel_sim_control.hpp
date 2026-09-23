#pragma once
#include "src/ui/common/panel_sim_control.hpp"
#include "src/sim/priest/parallel_runner.hpp"
#include "src/sim/priest/priest_sim.hpp"

namespace priest {

inline void render_priest_sim_control(PriestSimulator& sim,
                                      int& iterations,
                                      int& thread_count,
                                      BatchSimResult& last_result,
                                      bool& is_running,
                                      float& progress)
{
  warlock::render_common_sim_control<PriestSimulator, BatchSimResult, ParallelSimRunner>(
      sim, iterations, thread_count, last_result, is_running, progress, "RUN PRIEST DES SIMULATION");
}

} // namespace priest
