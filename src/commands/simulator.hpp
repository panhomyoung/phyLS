/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file simulator.hpp
 *
 * @brief Semi-tensor product based logic network simulation
 *
 * @author Homyoung
 * @since  2023/03/21
 */

#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include <time.h>

#include <fstream>

#include "../core/simulator/lut_parser.hpp"
#include "../core/simulator/simulator.hpp"

namespace alice {

class simulator_command : public command {
 public:
  explicit simulator_command(const environment::ptr &env)
      : command(env, "STP-based logic network simulation") {
    add_option("filename,-f", filename, "name of input file");
    add_flag("--verbose, -v", "verbose output");
  }

 protected:
  void execute() {
    std::ifstream ifs(filename);
    if (!ifs.good()) std::cerr << "Can't open file " << filename << std::endl;

    phyLS::CircuitGraph graph;
    phyLS::LutParser parser;

    if (!parser.parse(ifs, graph))
      std::cerr << "Can't parse file " << filename << std::endl;

    clock_t begin, end;
    double totalTime = 0.0;

    phyLS::simulator sim(graph);

    begin = clock();
    
    sim.simulate();

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    if (is_set("verbose")) sim.print_simulation_result();

    std::cout.setf(ios::fixed);
    std::cout << "[CPU time]   " << setprecision(3) << totalTime << " s"
              << std::endl;
  }

 private:
  std::string filename;
};

ALICE_ADD_COMMAND(simulator, "Verification")
}  // namespace alice

#endif
