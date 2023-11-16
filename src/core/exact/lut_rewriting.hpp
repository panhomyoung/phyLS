#ifndef LUT_REWRITING_HPP
#define LUT_REWRITING_HPP

#include <algorithm>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <percy/percy.hpp>
#include <string>
#include <vector>

#include "exact_dag.hpp"

using namespace percy;
using namespace mockturtle;

namespace phyLS {

struct lut_rewriting_params {
  /*! \brief Enable exact synthesis for XAG. */
  bool xag{false};
};

class lut_rewriting_manager {
 public:
  lut_rewriting_manager(klut_network klut, lut_rewriting_params const& ps)
      : klut(klut), ps(ps) {}

  klut_network run_c() {
    klut.foreach_node([&](auto const& n) {
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      std::vector<klut_network::node> lut_inputs;

      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      if (lut_inputs.size() <= 2) return true; /* continue */

      // exact synthesis for each node
      int input_num = lut_inputs.size();
      percy::chain chain;
      es(input_num, func, chain);
      std::vector<int> node;
      std::vector<int> right;
      std::vector<int> left;
      std::vector<std::string> tt;
      chain.bench_infor(node, left, right, tt);
      if (tt.size() != node.size()) {
        tt.pop_back();
        hex_invert(tt[tt.size() - 1]);
      }

      // rewrite origin node by the optimal Boolean chains of exact synthesis
      std::vector<mockturtle::klut_network::node> new_lut;
      for (int i = 0; i < node.size(); i++) {
        kitty::dynamic_truth_table node_tt(2u);
        kitty::create_from_hex_string(node_tt, tt[i]);
        if (left[i] <= input_num && right[i] <= input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[left[i] - 1], lut_inputs[right[i] - 1]}, node_tt);
          new_lut.push_back(node_new);
        } else if (left[i] <= input_num && right[i] > input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[left[i] - 1], new_lut[right[i] - 1 - input_num]},
              node_tt);
          new_lut.push_back(node_new);
        } else if (left[i] > input_num && right[i] > input_num) {
          const auto node_new =
              klut.create_node({new_lut[left[i] - 1 - input_num],
                                new_lut[right[i] - 1 - input_num]},
                               node_tt);
          new_lut.push_back(node_new);
        }
      }
      klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      return true;
    });

    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_dangling(klut);
    return klut_opt;
  }

  klut_network run_s() {
    klut.foreach_node([&](auto const& n) {
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      std::vector<klut_network::node> lut_inputs;

      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      if (lut_inputs.size() <= 2) return true; /* continue */

      // exact synthesis for each node
      int input_num = lut_inputs.size();
      phyLS::exact_lut_params ps;
      dag_impl mgr(func, input_num, ps);
      mgr.run_rewrite();

      // compute the techmap costs of all realizations
      float cost = 0;
      int min_cost_realization = 0;
      for (int i = 0; i < mgr.exact_synthesis_results.size(); i++) {
        float cost_temp = 0;
        for (auto y : mgr.exact_synthesis_results[i])
          cost_temp += bench_cost_area(y.tt);
        if (cost_temp < cost || cost == 0) {
          cost = cost_temp;
          min_cost_realization = i;
        }
      }
      std::vector<int> nodes;
      std::vector<int> rights;
      std::vector<int> lefts;
      std::vector<std::string> tts;
      for (auto x : mgr.exact_synthesis_results[min_cost_realization]) {
        nodes.push_back(x.node);
        lefts.push_back(x.left);
        rights.push_back(x.right);
        tts.push_back(x.tt);
      }

      // rewrite origin node by the optimal Boolean chains of exact synthesis
      std::vector<mockturtle::klut_network::node> new_lut;
      for (int i = nodes.size() - 1; i >= 0; i--) {
        kitty::dynamic_truth_table node_tt(2u);
        kitty::create_from_binary_string(node_tt, tts[i]);
        if (lefts[i] <= input_num && rights[i] <= input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[lefts[i] - 1], lut_inputs[rights[i] - 1]}, node_tt);
          new_lut.push_back(node_new);
        } else if (lefts[i] <= input_num && rights[i] > input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[lefts[i] - 1], new_lut[rights[i] - 1 - input_num]},
              node_tt);
          new_lut.push_back(node_new);
        } else if (lefts[i] > input_num && rights[i] > input_num) {
          const auto node_new =
              klut.create_node({new_lut[lefts[i] - 1 - input_num],
                                new_lut[rights[i] - 1 - input_num]},
                               node_tt);
          new_lut.push_back(node_new);
        }
      }
      klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      return true;
    });

    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_luts(klut);
    return klut_opt;
  }

  klut_network run_s_enu() {
    klut.foreach_node([&](auto const& n) {
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      std::vector<klut_network::node> lut_inputs;

      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      if (lut_inputs.size() <= 2) return true; /* continue */

      // exact synthesis for each node
      int input_num = lut_inputs.size();
      phyLS::exact_lut_params ps;
      dag_impl mgr(func, input_num, ps);
      mgr.run_rewrite_enu();

      // compute the techmap costs of all realizations
      float cost = 0;
      int min_cost_realization = 0;
      for (int i = 0; i < mgr.exact_synthesis_results.size(); i++) {
        float cost_temp = 0;
        for (auto y : mgr.exact_synthesis_results[i])
          cost_temp += bench_cost_area(y.tt);
        if (cost_temp < cost || cost == 0) {
          cost = cost_temp;
          min_cost_realization = i;
        }
      }
      std::vector<int> nodes;
      std::vector<int> rights;
      std::vector<int> lefts;
      std::vector<std::string> tts;
      for (auto x : mgr.exact_synthesis_results[min_cost_realization]) {
        nodes.push_back(x.node);
        lefts.push_back(x.left);
        rights.push_back(x.right);
        tts.push_back(x.tt);
      }

      // rewrite origin node by the optimal Boolean chains of exact synthesis
      std::vector<mockturtle::klut_network::node> new_lut;
      for (int i = nodes.size() - 1; i >= 0; i--) {
        kitty::dynamic_truth_table node_tt(2u);
        kitty::create_from_binary_string(node_tt, tts[i]);
        if (lefts[i] <= input_num && rights[i] <= input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[lefts[i] - 1], lut_inputs[rights[i] - 1]}, node_tt);
          new_lut.push_back(node_new);
        } else if (lefts[i] <= input_num && rights[i] > input_num) {
          const auto node_new = klut.create_node(
              {lut_inputs[lefts[i] - 1], new_lut[rights[i] - 1 - input_num]},
              node_tt);
          new_lut.push_back(node_new);
        } else if (lefts[i] > input_num && rights[i] > input_num) {
          const auto node_new =
              klut.create_node({new_lut[lefts[i] - 1 - input_num],
                                new_lut[rights[i] - 1 - input_num]},
                               node_tt);
          new_lut.push_back(node_new);
        }
      }
      klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      return true;
    });

    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_luts(klut);
    return klut_opt;
  }

 private:
  void hex_invert(std::string& tt_temp) {
    if (tt_temp == "1")
      tt_temp = "e";
    else if (tt_temp == "2")
      tt_temp = "d";
    else if (tt_temp == "3")
      tt_temp = "c";
    else if (tt_temp == "4")
      tt_temp = "b";
    else if (tt_temp == "5")
      tt_temp = "a";
    else if (tt_temp == "6")
      tt_temp = "9";
    else if (tt_temp == "7")
      tt_temp = "8";
    else if (tt_temp == "8")
      tt_temp = "7";
    else if (tt_temp == "9")
      tt_temp = "6";
    else if (tt_temp == "a")
      tt_temp = "5";
    else if (tt_temp == "b")
      tt_temp = "4";
    else if (tt_temp == "c")
      tt_temp = "3";
    else if (tt_temp == "d")
      tt_temp = "2";
    else if (tt_temp == "e")
      tt_temp = "1";
    else
      tt_temp = "0";
  }

  // TODO
  float techmap_cost(std::vector<std::vector<std::string>> tt) {
    float cost = 0;
    for (auto x : tt) {
      float cost_temp = 0;
      for (auto y : x) {
        cost_temp += bench_cost_area(y);
      }
      if (cost_temp < cost || cost == 0) cost = cost_temp;
    }
    return cost;
  }

  // TODO
  float bench_cost_area(std::string tt) {
    float area_cost = 0;
    if (tt == "0001")
      area_cost = 2;
    else if (tt == "0010")
      area_cost = 3;
    else if (tt == "0011")
      area_cost = 1;
    else if (tt == "0100")
      area_cost = 3;
    else if (tt == "0101")
      area_cost = 1;
    else if (tt == "0110")
      area_cost = 5;
    else if (tt == "0111")
      area_cost = 2;
    else if (tt == "1000")
      area_cost = 3;
    else if (tt == "1001")
      area_cost = 5;
    else if (tt == "1010")
      area_cost = 1;
    else if (tt == "1011")
      area_cost = 3;
    else if (tt == "1100")
      area_cost = 1;
    else if (tt == "1101")
      area_cost = 3;
    else if (tt == "1110")
      area_cost = 4;
    else
      area_cost = 0;
    return area_cost;
  }

  // TODO
  float bench_cost_depth(std::string tt) {
    float depth_cost = 0;
    if (tt == "0001")
      depth_cost = 1.4;
    else if (tt == "0010")
      depth_cost = 2.3;
    else if (tt == "0011")
      depth_cost = 0.9;
    else if (tt == "0100")
      depth_cost = 2.3;
    else if (tt == "0101")
      depth_cost = 0.9;
    else if (tt == "0110")
      depth_cost = 1.9;
    else if (tt == "0111")
      depth_cost = 1;
    else if (tt == "1000")
      depth_cost = 1.9;
    else if (tt == "1001")
      depth_cost = 2.1;
    else if (tt == "1010")
      depth_cost = 1;
    else if (tt == "1011")
      depth_cost = 1.9;
    else if (tt == "1100")
      depth_cost = 1;
    else if (tt == "1101")
      depth_cost = 1.9;
    else if (tt == "1110")
      depth_cost = 1.9;
    else
      depth_cost = 0;
    return depth_cost;
  }

  void es(int nr_in, std::string tt, percy::chain& result) {
    spec spec;
    chain c;
    spec.verbosity = 0;

    kitty::dynamic_truth_table f(nr_in);
    kitty::create_from_hex_string(f, tt);
    spec[0] = f;

    spec.preprocess();
    auto res = pd_ser_synthesize_parallel(spec, c, 4, "../src/pd/");
    if (res == success) result.copy(c);
  }

 private:
  klut_network klut;
  lut_rewriting_params const& ps;
};

klut_network lut_rewriting_c(klut_network& klut,
                             lut_rewriting_params const& ps = {}) {
  lut_rewriting_manager mgr(klut, ps);
  return mgr.run_c();
}

klut_network lut_rewriting_s(klut_network& klut,
                             lut_rewriting_params const& ps = {}) {
  lut_rewriting_manager mgr(klut, ps);
  return mgr.run_s();
}

klut_network lut_rewriting_s_enu(klut_network& klut,
                                 lut_rewriting_params const& ps = {}) {
  lut_rewriting_manager mgr(klut, ps);
  return mgr.run_s_enu();
}

}  // namespace phyLS

#endif
