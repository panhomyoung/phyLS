#include "simulator.hpp"

namespace phyLS {
simulator::simulator(CircuitGraph& graph) : graph(graph) {
  pattern_num = 10000;  // 随机产生10000个仿真向量
  // max_branch = int( log2(pattern_num) );                    //做cut的界
  max_branch = 8;
  sim_info.resize(graph.get_lines().size());  // 按照lines的id记录仿真向量的信息
  lines_flag.resize(graph.get_lines().size(), false);  // 按照线的id给线做标记
  for (const line_idx& line_id : graph.get_inputs()) {
    sim_info[line_id].resize(pattern_num);
    lines_flag[line_id] = true;
    for (int i = 0; i < pattern_num; i++) {
      sim_info[line_id][i] = rand() % 2;
    }
  }
}

bool simulator::simulate() {
  // 1: 给电路划分层级
  graph.match_logic_depth();
  // 2: 确定电路中需要仿真的nodes
  need_sim_nodes nodes = get_need_nodes();
  // 3: 仿真
  for (const auto& node : nodes) {
    single_node_sim(node);
  }
  return true;
}

need_sim_nodes simulator::get_need_nodes() {
  need_sim_nodes nodes;
  for (const auto& nodes_id : graph.get_m_node_level()) {
    for (const auto& node_id : nodes_id) {
      const auto& node = graph.get_gates()[node_id];
      const auto& output = graph.get_lines()[node.get_output()];
      // 如果是output节点或多扇出节点，则需要仿真
      if (output.is_output || output.destination_gates.size() > fanout_limit) {
        nodes.clear();
        lines_flag[node.get_output()] = true;  // 给需要仿真的节点打标记
        nodes.push_back(node_id);
        cut_tree(nodes);
      }
      nodes.push_back(node_id);
    }
  }
  nodes.clear();
  for (unsigned level = 0; level <= graph.get_mld(); level++) {
    for (const auto& node_id : graph.get_m_node_level()[level]) {
      line_idx output = graph.get_gates()[node_id].get_output();
      //lines_flag[output] = true;
      if (lines_flag[output] == true) {
        nodes.push_back(node_id);
      }
    }
  }
  return nodes;
}

// 广度优先遍历  cut tree
void simulator::cut_tree(need_sim_nodes& nodes) {
  need_sim_nodes temp_nodes;
  const auto& graph_lines = graph.get_lines();
  const auto& graph_gates = graph.get_gates();
  while (!nodes.empty()) {
    temp_nodes.clear();
    temp_nodes.push_back(nodes.front());
    nodes.pop_front();
    int count = 0;
    while (1) {
      for (const auto& input : graph_gates[temp_nodes.front()].get_inputs()) {
        count++;
        if (lines_flag[input] == false) {
          temp_nodes.push_back(graph_lines[input].source);
        }
      }
      temp_nodes.pop_front();
      count--;
      // 对应该两种情况 (确实是棵大树，将其化为小树 || 本就是一颗小树)
      if (count > max_branch || temp_nodes.empty()) {
        break;
      }
    }
    for (const auto& node_id : temp_nodes) {
      lines_flag[graph_gates[node_id].get_output()] = true;
      nodes.push_back(node_id);
    }
  }
}

void simulator::single_node_sim(const gate_idx node_id) {
  const auto& node = graph.get_gates()[node_id];
  const gate_idx& output = node.get_output();
  // 1:生成矩阵链
  std::map<line_idx, int> map;
  m_chain matrix_chain;
  get_node_matrix(node_id, matrix_chain, map);
  // 2:分析矩阵链，减少变量个数
  stp_logic_manage stp;
  stp_vec root_stp_vec = stp.normalize_matrix(matrix_chain);
  // 3:仿真
  std::vector<line_idx> variable(map.size());
  int inputs_number = variable.size();
  for (const auto& temp : map) variable[temp.second - 1] = temp.first;
  sim_info[output].resize(pattern_num);
  int idx;
  int bits = 1 << inputs_number;
  for (int i = 0; i < pattern_num; i++) {
    idx = 0;
    for (int j = 0; j < inputs_number; j++) {
      idx = (idx << 1) + sim_info[variable[j]][i];
    }
    idx = bits - idx;
    sim_info[output][i] = 1 - root_stp_vec(idx);
  }
  lines_flag[output] = true;
}

// 对于某个cut  生成matrix chain
void simulator::get_node_matrix(const gate_idx node_id, m_chain& mc,
                                std::map<line_idx, int>& map) {
  const auto& node = graph.get_gates()[node_id];
  mc.push_back(node.get_type());
  int temp;
  for (const auto& line_id : node.get_inputs()) {
    if (lines_flag[line_id])  // 存变量
    {
      if (map.find(line_id) == map.end()) {
        temp = map.size() + 1;
        map.emplace(line_id, map.size() + 1);
      } else
        temp = map.at(line_id);
      mc.emplace_back(1, temp);
    } else  // 存lut  (PIs)的lines_flag必为true 所以这种情况不可能出现source为空
      get_node_matrix(graph.get_lines()[line_id].source, mc, map);
  }
}

void simulator::print_simulation_result() {
  std::cout << "PI/PO : " << graph.get_inputs().size() << "/"
            << graph.get_outputs().size() << std::endl
            << "result size = " << sim_info.size() << std::endl;

  for (int i = 0; i < pattern_num; i++) {
    std::cout << "Pattern " << i + 1 << " : ";
    for (const auto& input_id : graph.get_inputs()) {
      std::cout << sim_info[input_id][i];
    }
    std::cout << " = ";
    for (const auto& output_id : graph.get_outputs()) {
      std::cout << sim_info[output_id][i];
    }
    std::cout << std::endl;
    std::cout << "random node result : " << sim_info[100][i] << std::endl;
  }
//   for (const auto& input_id : graph.get_inputs())
//     std::cout << graph.get_lines()[input_id].name << " ";
//   std::cout << ": ";
//   for (const auto& output_id : graph.get_outputs())
//     std::cout << graph.get_lines()[output_id].name << " ";
}

bool simulator::check_sim_info() {
  for (int i = 0; i < sim_info.size(); i++) {
    if (sim_info[i].size() != pattern_num) {
      std::cout << "!" << std::endl;
      return false;
    }
    for (int j = 0; j < sim_info[i].size(); j++) {
      if (sim_info[i][j] != 0 && sim_info[i][j] != 1) {
        std::cout << "!" << std::endl;
        return false;
      }
    }
  }
  std::cout << "^ ^" << std::endl;
  return true;
}
}  // namespace phyLS
