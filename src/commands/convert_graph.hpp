/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file convert_graph.hpp
 *
 * @brief performs decomposable function for resynthesis graph
 *
 * @author Homyoung
 * @since  2022/12/20
 */

#ifndef CONVERT_GRAPH_HPP
#define CONVERT_GRAPH_HPP

#include <iostream>
#include <mockturtle/algorithms/klut_to_graph.hpp>

using namespace std;
using namespace mockturtle;

namespace alice {

class create_graph_command : public command {
 public:
  explicit create_graph_command(const environment::ptr& env)
      : command(env, "functional reduction [default = AIG]") {
    add_option("-e,--expression", expression,
               "creates new graph from expression");
    add_flag("--mig, -m", "functional reduction for MIG");
    add_flag("--xag, -g", "functional reduction for XAG");
    add_flag("--xmg, -x", "functional reduction for XMG");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    auto& opt_ntks = store<optimum_network>();
    if (opt_ntks.empty()) opt_ntks.extend();

    uint32_t num_vars{0u};
    for (auto c : expression) {
      if (c >= 'a' && c <= 'p')
        num_vars = std::max<uint32_t>(num_vars, c - 'a' + 1u);
    }

    kitty::dynamic_truth_table tt(num_vars);
    kitty::create_from_expression(tt, expression);

    klut_network kLUT_ntk;
    vector<klut_network::signal> children;
    for (auto i = 0; i < num_vars; i++) {
      const auto x = kLUT_ntk.create_pi();
      children.push_back(x);
    }

    auto fn = [&](kitty::dynamic_truth_table const& remainder,
                  std::vector<klut_network::signal> const& children) {
      return kLUT_ntk.create_node(children, remainder);
    };

    kLUT_ntk.create_po(dsd_decomposition(kLUT_ntk, tt, children, fn));

    if (is_set("mig")) {
      auto mig = convert_klut_to_graph<mig_network>(kLUT_ntk);
      phyLS::print_stats(mig);
      store<mig_network>().extend();
      store<mig_network>().current() = mig;
    } else if (is_set("xmg")) {
      auto xmg = convert_klut_to_graph<xmg_network>(kLUT_ntk);
      phyLS::print_stats(xmg);
      store<xmg_network>().extend();
      store<xmg_network>().current() = xmg;
    } else if (is_set("xag")) {
      auto xag = convert_klut_to_graph<xag_network>(kLUT_ntk);
      phyLS::print_stats(xag);
      store<xag_network>().extend();
      store<xag_network>().current() = xag;
    } else {
      auto aig = convert_klut_to_graph<aig_network>(kLUT_ntk);
      phyLS::print_stats(aig);
      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }
    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  string expression = "";
};

ALICE_ADD_COMMAND(create_graph, "Synthesis")

}  // namespace alice

#endif