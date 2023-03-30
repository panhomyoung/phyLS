#ifndef LUT_PARSER_HPP
#define LUT_PARSER_HPP

#include "circuit_graph.hpp"
#include "myfunction.hpp"

namespace phyLS {
class LutParser {
 public:
  bool parse(std::istream& is, CircuitGraph& graph);

 private:
  bool match_input(CircuitGraph& graph, const std::string& line);
  bool match_output(CircuitGraph& graph, const std::string& line);
  bool match_gate(CircuitGraph& graph, const std::string& line);
  Type get_stp_vec(const std::string& tt, const int& inputs_num);
};
}  // namespace phyLS

#endif
