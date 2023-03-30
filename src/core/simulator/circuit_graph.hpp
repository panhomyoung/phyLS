#ifndef CIRCUIT_GRAPH_H
#define CIRCUIT_GRAPH_H

#include <cassert>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "stp_vector.hpp"

#define NULL_INDEX -1
#define NO_LEVEL -10000

namespace phyLS {

using gate_idx = int;
using line_idx = int;
using node_level = int;

using Type = stp_vec;
class Gate;
class CircuitGraph;

struct Line {
  void connect_as_input(gate_idx gate) { destination_gates.insert(gate); }

  gate_idx source = NULL_INDEX;  // nullptr means input port
  std::set<gate_idx> destination_gates;
  bool is_input = false;
  bool is_output = false;
  int id_line = NULL_INDEX;
  std::string name;
};

class Gate {
 public:
  Gate(Type type, line_idx output,
       std::vector<line_idx> &&inputs);  // 构造一般gate

  Type get_type() const { return m_type; }
  Type &type() { return m_type; }

  const std::vector<line_idx> &get_inputs() const { return m_inputs; }
  std::vector<line_idx> &inputs() { return m_inputs; }

  const line_idx &get_output() const { return m_output; }
  line_idx &output() { return m_output; }

  const int &get_id() const { return m_id; }
  int &id() { return m_id; }

  const int &get_level() const { return m_level; }
  int &level() { return m_level; }

  bool is_input() const { return m_type.cols() == 0; }

  int make_gate_name(Type type);

 private:
  Type m_type;
  int m_id = NULL_INDEX;
  node_level m_level = NO_LEVEL;
  std::vector<line_idx> m_inputs;
  line_idx m_output = NULL_INDEX;
};

class CircuitGraph {
 public:
  CircuitGraph();

  line_idx add_input(const std::string &name);
  line_idx add_output(const std::string &name);
  gate_idx add_gate(Type type, const std::vector<std::string> &input_names,
                    const std::string &output_name);

  const line_idx get_line(const std::string &name) const;
  line_idx line(const std::string &name);

  const Gate &get_gate(const gate_idx &idx) const { return m_gates[idx]; }
  Gate &gate(const gate_idx &idx) { return m_gates[idx]; }

  const Line &get_line(const line_idx &idx) const { return m_lines[idx]; }
  Line &line(const line_idx &idx) { return m_lines[idx]; }

  const std::vector<line_idx> &get_inputs() const;
  std::vector<line_idx> &inputs() { return m_inputs; }

  const std::vector<line_idx> &get_outputs() const;
  std::vector<line_idx> &outputs() { return m_outputs; }

  const std::vector<Gate> &get_gates() const;
  std::vector<Gate> &get_gates();

  const std::vector<Line> &get_lines() const;
  std::vector<Line> &lines() { return m_lines; }

  const std::vector<std::vector<gate_idx>> &get_m_node_level() const {
    return m_node_level;
  }
  std::vector<std::vector<gate_idx>> &get_m_node_level() {
    return m_node_level;
  }

  const int &get_mld() const { return max_logic_depth; }

  void match_logic_depth();
  void print_graph();

 private:
  line_idx ensure_line(const std::string &name);
  int compute_node_depth(const gate_idx g_id);

 private:
  std::vector<Line> m_lines;
  std::vector<Gate> m_gates;

  std::vector<line_idx> m_inputs;
  std::vector<line_idx> m_outputs;

  std::vector<std::vector<gate_idx>> m_node_level;
  int max_logic_depth = -1;

 public:
  std::unordered_map<std::string, line_idx> m_name_to_line_idx;
};
}  // namespace phyLS

#endif