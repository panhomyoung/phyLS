#include "lut_parser.hpp"

#include <algorithm>
#include <string>

namespace phyLS {
bool LutParser::parse(std::istream& is, CircuitGraph& graph) {
  const std::string flag_input = "INPUT";
  const std::string flag_output = "OUTPUT";
  const std::string flag_lut = "LUT";
  const std::string flag_gnd = "gnd";
  for (std::string line; std::getline(is, line, '\n');) {
    if (line.empty()) continue;
    if (line.find(flag_input) != std::string::npos) {
      match_input(graph, line);
      continue;
    }
    if (line.find(flag_output) != std::string::npos) {
      match_output(graph, line);
      continue;
    }
    if (line.find(flag_lut) != std::string::npos) {
      match_gate(graph, line);
      continue;
    }
  }
  return true;
}

bool LutParser::match_input(CircuitGraph& graph, const std::string& line) {
  std::string input_name = m_split(line, "( )")[1];
  graph.add_input(input_name);
  return true;
}

bool LutParser::match_output(CircuitGraph& graph, const std::string& line) {
  std::string output_name = m_split(line, "( )")[1];
  graph.add_output(output_name);
  return true;
}

bool LutParser::match_gate(CircuitGraph& graph, const std::string& line) {
  std::vector<std::string> gate = m_split(line, ",=( )");
  std::string output = gate[0];
  std::string tt = gate[2];
  gate.erase(gate.begin(), gate.begin() + 3);
  std::vector<std::string> inputs(gate);
  tt.erase(0, 2);  // 删除0x
  Type type = get_stp_vec(tt, inputs.size());
  graph.add_gate(type, inputs, output);
  return true;
}

stp_vec LutParser::get_stp_vec(const std::string& tt, const int& inputs_num) {
  // 只有一个输入的节点(buff 或 not)
  if (inputs_num == 1 && tt.size() == 1) {
    stp_vec type(3);
    type(0) = 2;
    switch (tt[0]) {
      case '0':
        type(1) = 1;
        type(2) = 1;
        break;
      case '1':
        type(1) = 1;
        type(2) = 0;
        break;
      case '2':
        type(1) = 0;
        type(2) = 1;
        break;
      case '3':
        type(1) = 0;
        type(2) = 0;
        break;
      default:
        break;
    }
    return type;
  }
  stp_vec type((1 << inputs_num) + 1);
  type(0) = 2;
  int type_idx;
  for (int i = 0, len = tt.size(); i < len; i++) {
    type_idx = 4 * i + 1;
    switch (tt[i]) {
      case '0':  // 0000 - > 1111
        type(type_idx) = 1;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 1;
        break;
      case '1':  // 0001 - > 1110
        type(type_idx) = 1;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 0;
        break;
      case '2':  // 0010 - > 1101
        type(type_idx) = 1;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 1;
        break;
      case '3':  // 0011 - > 1100
        type(type_idx) = 1;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 0;
        break;
      case '4':  // 0100 - > 1011
        type(type_idx) = 1;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 1;
        break;
      case '5':  // 0101 - > 1010
        type(type_idx) = 1;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 0;
        break;
      case '6':  // 0110 - > 1001
        type(type_idx) = 1;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 1;
        break;
      case '7':  // 0111 - > 1000
        type(type_idx) = 1;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 0;
        break;
      case '8':  // 1000 - > 0111
        type(type_idx) = 0;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 1;
        break;
      case '9':  // 1001 - > 0110
        type(type_idx) = 0;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 0;
        break;
      case 'a':  // 1010 - > 0101
        type(type_idx) = 0;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 1;
        break;
      case 'b':  // 1011 - > 0100
        type(type_idx) = 0;
        type(type_idx + 1) = 1;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 0;
        break;
      case 'c':  // 1100 - > 0011
        type(type_idx) = 0;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 1;
        break;
      case 'd':  // 1101 - > 0010
        type(type_idx) = 0;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 1;
        type(type_idx + 3) = 0;
        break;
      case 'e':  // 1110 - > 0001
        type(type_idx) = 0;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 1;
        break;
      case 'f':  // 1111 - > 0000
        type(type_idx) = 0;
        type(type_idx + 1) = 0;
        type(type_idx + 2) = 0;
        type(type_idx + 3) = 0;
        break;
      default:
        break;
    }
  }
  return type;
}
}  // namespace phyLS