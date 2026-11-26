#include <exception>
#include <iostream>

#include "Simulator.h"

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cerr
        << "Usage: engine_smoke CONFIG ONNX ATTRIBUTE\n";
    return 1;
  }

  try {
    auto simulator =
        Simulator::from_config_file(argv[1]);

    if (!simulator) {
      std::cerr << "Failed to create simulator\n";
      return 1;
    }

    simulator->enqueue_graph_from_files(
        0,        // Kernel ID
        argv[2],  // ONNX path
        argv[3],  // Attribute path
        0,        // Request time
        0);       // Partition ID

    // Execute the first event group.
    const auto first = simulator->next_event_time_ps();

    if (!first || !simulator->step() ||
        simulator->current_time_ps() != *first) {
    std::cerr << "STEP_FAIL\n";
    return 1;
    }

    // Verify that advance_to() cannot execute a future event.
    const auto next = simulator->next_event_time_ps();

    if (!next || *next <= simulator->current_time_ps()) {
    std::cerr << "NEXT_EVENT_FAIL\n";
    return 1;
    }

    if (simulator->advance_to(*next - 1) != 0) {
    std::cerr << "EVENT_BOUNDARY_FAIL\n";
    return 1;
    }

    // Execute the remaining workload in bounded time windows.
    uint64_t iterations = 0;

    while (!simulator->idle()) {
    const auto next_event = simulator->next_event_time_ps();

    if (!next_event) {
        std::cerr << "MISSING_EVENT_FAIL\n";
        return 1;
    }

    const uint64_t target = *next_event + 100000;

    const auto executed = simulator->advance_to(target);

    const auto pending = simulator->next_event_time_ps();

    if (executed == 0 ||
        simulator->current_time_ps() > target ||
        (pending && *pending <= target)) {
        std::cerr << "ADVANCE_TO_FAIL\n";
        return 1;
    }

    ++iterations;
    }

    // Preserve the existing final tag check.
    simulator->cycle();

    const auto cycles = simulator->get_core_cycle();

    std::cout << "Final event time (ps): "
            << simulator->current_time_ps() << '\n';

    std::cout << "Advance iterations: "
            << iterations << '\n';

    std::cout << "BOUNDED_EXECUTION_PASS\n";

    simulator->print_core_stat();

    std::cout << "Engine cycles: " << cycles << '\n';

    if (cycles != 532) {
      std::cerr << "ENGINE_SMOKE_FAIL\n";
      return 1;
    }

    std::cout << "ENGINE_SMOKE_PASS\n";

  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}