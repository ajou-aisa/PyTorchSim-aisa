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

    simulator->cycle();

    const auto cycles = simulator->get_core_cycle();

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