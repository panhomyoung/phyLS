#ifndef C_SIMLUATOR_HPP
#define C_SIMLUATOR_HPP

#include <deque>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "circuit_graph.hpp"
#include "myfunction.hpp"

namespace phyLS {

using need_sim_nodes = std::deque<gate_idx>;
using line_sim_info = std::vector<u_int16_t>;

class simulator {
 public:
  simulator(CircuitGraph& graph);
  bool simulate();  // simulate
  bool full_simulate();  //full_simulate
  std::vector<std::vector<int>> generateBinary(int n);
  void print_simulation_result();

 private:
  bool is_simulated(const line_idx id) {
    return sim_info[id].size() == pattern_num;
  }

  need_sim_nodes get_need_nodes();
  void single_node_sim(const gate_idx node);
  void get_node_matrix(const gate_idx node, m_chain& mc,
                       std::map<line_idx, int>& map);
  void cut_tree(need_sim_nodes& nodes);
  bool check_sim_info();

 private:
  std::vector<line_sim_info> sim_info;
  std::vector<int> time_interval;
  std::vector<bool> lines_flag;
  CircuitGraph& graph;
  int pattern_num;
  int max_branch;
  int fanout_limit = 1;
};
}  // namespace phyLS

#endif