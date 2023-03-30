#include "circuit_graph.hpp"

#include <iostream>
#include <map>
#include <set>
#include <sstream>

namespace phyLS {
Gate::Gate(Type type, line_idx output, std::vector<line_idx>&& inputs)
    : m_type(type), m_inputs(inputs), m_output(output) {}

int Gate::make_gate_name(Type type) {
  int lut = 0;
  for (int i = 1; i < type.cols(); i++) {
    lut = (lut << 1) + 1 - type(i);
  }
  return lut;
}

// 图的构造函数，预留一块空间
CircuitGraph::CircuitGraph() {
  m_gates.reserve(5000u);
  m_lines.reserve(5000u);
}

line_idx CircuitGraph::add_input(const std::string& name) {
  line_idx p_line = ensure_line(name);
  if (!m_lines[p_line].is_input) {
    m_lines[p_line].is_input = true;
    m_inputs.push_back(p_line);
  }
  return p_line;
}

line_idx CircuitGraph::add_output(const std::string& name) {
  line_idx p_line = ensure_line(name);
  if (!m_lines[p_line].is_output) {
    m_lines[p_line].is_output = true;
    m_outputs.push_back(p_line);
  }
  return p_line;
}

gate_idx CircuitGraph::add_gate(Type type,
                                const std::vector<std::string>& input_names,
                                const std::string& output_name) {
  std::vector<line_idx> inputs;
  // for (size_t i = 0; i < input_names.size(); ++i)
  for (int i = input_names.size() - 1; i >= 0; i--) {
    line_idx p_input = ensure_line(input_names[i]);
    inputs.push_back(p_input);
  }

  line_idx p_output = ensure_line(output_name);

  m_gates.emplace_back(type, p_output, std::move(inputs));
  gate_idx gate = m_gates.size() - 1;
  m_lines[p_output].source = gate;
  m_gates.back().id() = gate;

  for (size_t i = 0; i < m_gates[gate].get_inputs().size(); ++i) {
    m_lines[m_gates[gate].get_inputs().at(i)].connect_as_input(gate);
  }
  return gate;
}

line_idx CircuitGraph::line(const std::string& name) {
  auto it = m_name_to_line_idx.find(name);

  if (it != m_name_to_line_idx.end()) {
    return it->second;
  }

  return NULL_INDEX;
}

const line_idx CircuitGraph::get_line(const std::string& name) const {
  auto it = m_name_to_line_idx.find(name);

  if (it != m_name_to_line_idx.end()) {
    return it->second;
  }
  return NULL_INDEX;
}

const std::vector<line_idx>& CircuitGraph::get_inputs() const {
  return m_inputs;
}

const std::vector<line_idx>& CircuitGraph::get_outputs() const {
  return m_outputs;
}

const std::vector<Gate>& CircuitGraph::get_gates() const { return m_gates; }

std::vector<Gate>& CircuitGraph::get_gates() { return m_gates; }

const std::vector<Line>& CircuitGraph::get_lines() const { return m_lines; }

line_idx CircuitGraph::ensure_line(const std::string& name) {
  auto it = m_name_to_line_idx.find(name);

  if (it != m_name_to_line_idx.end()) {
    return it->second;
  }

  m_lines.emplace_back();
  Line& line = m_lines.back();

  line.name = name;
  line.id_line = m_lines.size() - 1;

  m_name_to_line_idx[name] = m_lines.size() - 1;

  return line.id_line;
}

void CircuitGraph::match_logic_depth() {
  for (int i = 0, num = m_outputs.size(); i < num; i++) {
    int level = compute_node_depth(m_lines[m_outputs[i]].source);
    if (level > max_logic_depth) max_logic_depth = level;
  }
  m_node_level.resize(max_logic_depth + 1);
  for (int i = 0; i < m_gates.size(); i++) {
    m_node_level[m_gates[i].get_level()].push_back(i);
  }
}

// 计算该节点的depth = max children's depth
int CircuitGraph::compute_node_depth(const gate_idx g_id) {
  Gate& gate = m_gates[g_id];
  // 如果已经被计算过  直接返回
  if (gate.get_level() != NO_LEVEL) return gate.get_level();
  // 访问所有子节点计算
  int max_depth = NO_LEVEL;
  int level = -1;
  for (const auto& child : gate.get_inputs()) {
    if (m_lines[child].is_input) continue;
    level = compute_node_depth(m_lines[child].source);
    if (level > max_depth) {
      max_depth = level;
    }
  }
  if (max_depth ==
      NO_LEVEL)  // 走出for循环max_depth没有改变，说明该节点所有的输入线都是input
    max_depth = -1;
  m_gates[g_id].level() = max_depth + 1;
  return m_gates[g_id].level();
}

void CircuitGraph::print_graph() {
  // 打印INPUT
  for (unsigned i = 0, length = m_inputs.size(); i < length; i++) {
    std::cout << "INPUT(" << m_lines[m_inputs[i]].name << ")" << std::endl;
  }
  // 打印OUTPUT
  for (unsigned i = 0, length = m_outputs.size(); i < length; i++) {
    std::cout << "OUTPUT(" << m_lines[m_outputs[i]].name << ")" << std::endl;
  }
  // 打印gate
  for (unsigned i = 0, length = m_gates.size(); i < length; i++) {
    auto& gate = m_gates[i];
    std::cout << m_lines[gate.get_output()].name << " = LUT 0x" << std::hex
              << int(gate.make_gate_name(gate.get_type())) << "(";
    std::vector<std::string> inputs_name;
    // for(const auto& input : gate.get_inputs())
    for (int i = gate.get_inputs().size() - 1; i > -1;) {
      inputs_name.push_back(m_lines[gate.get_inputs()[i]].name);
      inputs_name.push_back(", ");
    }
    inputs_name.pop_back();
    for (const auto& temp : inputs_name) std::cout << temp;
    std::cout << ")" << std::endl;
  }

  // //打印逻辑深度的信息
  // int level = -1;
  // std::cout << "*************************************" << std::endl;
  // for(const auto& t1 : m_node_level)
  // {
  // 	level++;
  // 	std::cout << "lev = " << level << " : ";
  // 	for(const auto& t2 : t1)
  // 	{
  // 		std::cout << t2 << " ";
  // 	}
  // 	std::cout << std::endl;
  // }

  // //遍历所有的线 打印线的前后节点信息
  // for(const auto& line : m_lines)
  // {
  // 	std::cout << "*************************************" << std::endl;
  // 	std::cout << "name: " << line.name << "   ";
  // 	if(line.is_input)
  // 		std::cout << "input" << "    /";
  // 	if(line.is_output)
  // 		std::cout << "output" << "    /";
  // 	if(!line.is_input)
  // 		std::cout << "source:    " << line.source << std::endl;
  // 	if(!line.is_output)
  // 	{
  // 		std::cout << "destination_gates:  ";
  // 		for(const auto& gate : line.destination_gates)
  // 			std::cout << gate << " ";
  // 		std::cout << std::endl;
  // 	}
  // }
}
}  // namespace phyLS
