#ifndef EXACT_LUT_MAPPING_HPP
#define EXACT_LUT_MAPPING_HPP

#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <percy/percy.hpp>
#include <string>
#include <thread>
#include <vector>

#include "../flow_detail.hpp"
#include "exact_lut.hpp"

using namespace percy;
using namespace mockturtle;

namespace phyLS {

struct exact_lut_mapping_params {
  int cut_size = 4;
};

class exact_lut_mapping_manager {
 public:
  exact_lut_mapping_manager(klut_network klut,
                            exact_lut_mapping_params const& ps)
      : klut(klut), ps(ps) {}

  klut_network run() {
    std::map<std::string, vector<phyLS::klut>> exact_lut_results;
    klut.foreach_node([&](auto n) {
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      vector<string> funcs;
      funcs.push_back(hexToBinary(func));
      std::vector<node<klut_network>> lut_inputs;
      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      int cut_size = ps.cut_size;
      if (lut_inputs.size() <= cut_size) return true; /* continue */
      int input_num = lut_inputs.size();
      std::vector<int> node;
      std::vector<vector<int>> inputs;
      std::vector<std::string> tt;
      phyLS::exact_lut_impl mgr(funcs, input_num, cut_size);
      if (exact_lut_results.find(func) != exact_lut_results.end()) {
        for (auto x : exact_lut_results[func]) {
          node.push_back(x.node);
          inputs.push_back(x.inputs);
          tt.push_back(x.tt);
        }
      } else {
        mgr.run_lut_mapping();
        for (auto x : mgr.exact_lut_result) {
          node.push_back(x.node);
          inputs.push_back(x.inputs);
          tt.push_back(x.tt);
        }
        exact_lut_results[func] = mgr.exact_lut_result;
      }
      std::vector<mockturtle::klut_network::node> new_lut;
      for (int i = node.size() - 1; i >= 0; i--) {
        kitty::dynamic_truth_table node_tt(cut_size);
        kitty::create_from_binary_string(node_tt, tt[i]);
        std::vector<mockturtle::klut_network::signal> children;
        for (auto x : inputs[i]) {
          if (x <= input_num) {
            children.push_back(lut_inputs[x - 1]);
          } else {
            children.push_back(new_lut[x - 1 - input_num]);
          }
        }
        const auto node_new = klut.create_node(children, node_tt);
        new_lut.push_back(node_new);
      }
      klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      return true;
    });
    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_luts(klut);
    return klut_opt;
  }

  klut_network run_mix() {
    std::map<std::string, vector<phyLS::klut>> exact_lut_results;
    int t_num = 0;
    klut.foreach_node([&](auto n) {
      timeout = false;
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      vector<string> funcs;
      funcs.push_back(hexToBinary(func));
      std::vector<node<klut_network>> lut_inputs;
      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      int cut_size = ps.cut_size;
      if (lut_inputs.size() <= cut_size) return true; /* continue */
    //   std::cout << "func: " << func << "  ";
      int input_num = lut_inputs.size();

      int num_pi = 0;
      std::vector<int> node;
      std::vector<vector<int>> inputs;
      std::vector<std::string> tt;

      std::vector<int> node_dec;
      std::vector<vector<int>> inputs_dec;
      std::vector<kitty::dynamic_truth_table> tt_dec;

      phyLS::exact_lut_impl mgr(funcs, input_num, cut_size);
      if (exact_lut_results.find(func) != exact_lut_results.end()) {
        // std::cout << "find result." << "  ";
        for (auto x : exact_lut_results[func]) {
          node.push_back(x.node);
          inputs.push_back(x.inputs);
          tt.push_back(x.tt);
        }
        timeout = true;
      } else {
        std::packaged_task<void()> task(
            [this, &mgr] { mgr.run_lut_mapping(); });
        futures.push_back(task.get_future());
        threads.emplace_back(std::move(task));
        if (futures[t_num].wait_for(std::chrono::milliseconds(100)) ==
            std::future_status::timeout) {
          threads[t_num].detach();  // 超时情况下分离线程
        } else {
          threads[t_num].join();  // 正常情况下等待线程完成
          timeout = true;
        }
        t_num++;

        // std::cout << "timeout: " << timeout << "   ";
        if (timeout) {
        //   std::cout << "exact synthesis." << "  ";
          for (auto x : mgr.exact_lut_result) {
            node.push_back(x.node);
            inputs.push_back(x.inputs);
            tt.push_back(x.tt);
          }
          exact_lut_results[func] = mgr.exact_lut_result;
        } else {
        //   std::cout << "decomposition." << "  ";
          int var_num;
          mockturtle::klut_network ntk;
          std::vector<mockturtle::klut_network::signal> children;
          kitty::dynamic_truth_table remainder;
          mockturtle::decomposition_flow_params ps1;
          mockturtle::read_hex(func, remainder, var_num, ntk, children);
          ntk.create_po(mockturtle::dsd_detail(ntk, remainder, children, ps1));
          num_pi = ntk.num_pis() + 1;
          ntk.foreach_node([&](auto const& y) {
            if (y > num_pi) {
              node_dec.push_back(y);
              vector<int> input;
              ntk.foreach_fanin(
                  y, [&](auto const& child) { input.push_back(child); });
              inputs_dec.push_back(input);
              tt_dec.push_back(ntk.node_function(y));
            }
          });
        }
      }
      
      // rewrite origin LUT by local exact LUT mapping
      std::vector<mockturtle::klut_network::node> new_lut;
      if (timeout) {
        // std::cout << "exact synthesis klut." << "  ";
        for (int i = node.size() - 1; i >= 0; i--) {
          kitty::dynamic_truth_table node_tt(cut_size);
          kitty::create_from_binary_string(node_tt, tt[i]);
          std::vector<mockturtle::klut_network::signal> children;
          for (auto x : inputs[i]) {
            if (x <= input_num) {
              children.push_back(lut_inputs[x - 1]);
            } else {
              children.push_back(new_lut[x - 1 - input_num]);
            }
          }
          const auto node_new = klut.create_node(children, node_tt);
          new_lut.push_back(node_new);
        }
        klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      } else {
        // std::cout << "decomposition klut." << "  ";
        for (int i = 0; i < node_dec.size(); i++) {
          std::vector<mockturtle::klut_network::signal> children;
          for (auto x : inputs_dec[i]) {
            if (x <= num_pi) {
              children.push_back(lut_inputs[x - 2]);
            } else {
              children.push_back(new_lut[x - 1 - num_pi]);
            }
          }
          const auto node_new = klut.create_node(children, tt_dec[i]);
          new_lut.push_back(node_new);
        }
        klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      }
    //   std::cout << "finish" << std::endl;
      return true;
    });
    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_luts(klut);
    return klut_opt;
  }

  klut_network run_dec() {
    klut.foreach_node([&](auto n) {
      if (klut.is_constant(n) || klut.is_pi(n)) return true; /* continue */
      std::string func = kitty::to_hex(klut.node_function(n));
      std::vector<node<klut_network>> lut_inputs;
      klut.foreach_fanin(
          n, [&](auto const& c) { lut_inputs.push_back(klut.get_node(c)); });
      int cut_size = ps.cut_size;
      if (lut_inputs.size() <= cut_size) return true; /* continue */
    //   std::cout << "func: " << func << "  ";

      int var_num;
      mockturtle::klut_network ntk;
      std::vector<mockturtle::klut_network::signal> children;
      kitty::dynamic_truth_table remainder;
      mockturtle::decomposition_flow_params ps1;
      mockturtle::read_hex(func, remainder, var_num, ntk, children);
      ntk.create_po(mockturtle::dsd_detail(ntk, remainder, children, ps1));
    //   std::cout << "decomposition klut." << "  ";

      std::vector<int> node;
      std::vector<vector<int>> inputs;
      std::vector<kitty::dynamic_truth_table> tt;

      int num_pi = ntk.num_pis() + 1;
      ntk.foreach_node([&](auto const& y) {
        if (y > num_pi) {
          node.push_back(y);
          vector<int> input;
          ntk.foreach_fanin(y,
                            [&](auto const& child) { input.push_back(child); });
          inputs.push_back(input);
          tt.push_back(ntk.node_function(y));
        }
      });

      std::vector<mockturtle::klut_network::node> new_lut;
      for (int i = 0; i < node.size(); i++) {
        std::vector<mockturtle::klut_network::signal> children;
        for (auto x : inputs[i]) {
          if (x <= num_pi) {
            children.push_back(lut_inputs[x - 2]);
          } else {
            children.push_back(new_lut[x - 1 - num_pi]);
          }
        }
        const auto node_new = klut.create_node(children, tt[i]);
        new_lut.push_back(node_new);
      }
      klut.substitute_node(n, new_lut[new_lut.size() - 1]);
      return true;
    });
    // clean all redundant luts
    auto klut_opt = mockturtle::cleanup_luts(klut);
    return klut_opt;
  }

 private:
  std::string hexToBinary(std::string tt) {
    std::string tt_result;
    for (auto x : tt) {
      if (x == '0')
        tt_result += "0000";
      else if (x == '1')
        tt_result += "0001";
      else if (x == '2')
        tt_result += "0010";
      else if (x == '3')
        tt_result += "0011";
      else if (x == '4')
        tt_result += "0100";
      else if (x == '5')
        tt_result += "0101";
      else if (x == '6')
        tt_result += "0110";
      else if (x == '7')
        tt_result += "0111";
      else if (x == '8')
        tt_result += "1000";
      else if (x == '9')
        tt_result += "1001";
      else if (x == 'a')
        tt_result += "1010";
      else if (x == 'b')
        tt_result += "1011";
      else if (x == 'c')
        tt_result += "1100";
      else if (x == 'd')
        tt_result += "1101";
      else if (x == 'e')
        tt_result += "1110";
      else
        tt_result += "1111";
    }
    return tt_result;
  }


 private:
  klut_network klut;
  exact_lut_mapping_params const& ps;
  bool timeout = false;
  std::vector<std::thread> threads;
  std::vector<std::future<void>> futures;
};

klut_network exact_lut_mapping(klut_network& klut,
                               exact_lut_mapping_params const& ps = {}) {
  exact_lut_mapping_manager mgr(klut, ps);
  return mgr.run();
}

klut_network exact_lut_mapping_dec(klut_network& klut,
                               exact_lut_mapping_params const& ps = {}) {
  exact_lut_mapping_manager mgr(klut, ps);
  return mgr.run_dec();
}

klut_network exact_lut_mapping_mix(klut_network& klut,
                               exact_lut_mapping_params const& ps = {}) {
  exact_lut_mapping_manager mgr(klut, ps);
  return mgr.run_mix();
}

}  // namespace phyLS

#endif
