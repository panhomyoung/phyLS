/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file algebraic_rewriting.hpp
 *
 * @brief algebraic depth rewriting
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef ALGEBRAIC_REWRITING_HPP
#define ALGEBRAIC_REWRITING_HPP

#include <mockturtle/algorithms/mig_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/xag_algebraic_rewriting.hpp>
#include <mockturtle/algorithms/xmg_algebraic_rewriting.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace std;
using namespace mockturtle;

namespace alice {

class algebraic_rewriting_command : public command {
 public:
  explicit algebraic_rewriting_command(const environment::ptr& env)
      : command(env, "algebraic depth rewriting [default = MIG]") {
    add_flag("--xag, -g", "refactoring for XAG");
    add_flag("--xmg, -x", "refactoring for XMG");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    if (is_set("xag")) {
      if (store<xag_network>().size() == 0u)
        std::cerr << "Error: Empty XAG network\n";
      else {
        auto xag = store<xag_network>().current();
        depth_view depth_xag{xag};
        xag_algebraic_depth_rewriting_params ps;
        ps.allow_rare_rules = true;
        xag_algebraic_depth_rewriting(depth_xag, ps);
        depth_xag = cleanup_dangling(depth_xag);
        phyLS::print_stats(depth_xag);
        store<xag_network>().extend();
        store<xag_network>().current() = depth_xag;
      }
    } else if (is_set("xmg")) {
      if (store<xmg_network>().size() == 0u)
        std::cerr << "Error: Empty XMG network\n";
      else {
        auto xmg = store<xmg_network>().current();
        depth_view depth_xmg{xmg};
        xmg_algebraic_depth_rewriting_params ps;
        ps.strategy = xmg_algebraic_depth_rewriting_params::selective;
        xmg_algebraic_depth_rewriting(depth_xmg, ps);
        depth_xmg = cleanup_dangling(depth_xmg);
        phyLS::print_stats(depth_xmg);
        store<xmg_network>().extend();
        store<xmg_network>().current() = depth_xmg;
      }
    } else {
      if (store<mig_network>().size() == 0u)
        std::cerr << "Error: Empty MIG network\n";
      else {
        auto mig = store<mig_network>().current();
        depth_view depth_mig{mig};
        mig_algebraic_depth_rewriting_params ps;
        mig_algebraic_depth_rewriting(depth_mig, ps);
        depth_mig = cleanup_dangling(depth_mig);
        phyLS::print_stats(depth_mig);
        store<mig_network>().extend();
        store<mig_network>().current() = depth_mig;
      }
    }
    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(algebraic_rewriting, "Logic synthesis")

}  // namespace alice

#endif