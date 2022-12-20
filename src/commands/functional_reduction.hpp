/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file functional_reduction.hpp
 *
 * @brief performs functional reduction
 *
 * @author Homyoung
 * @since  2022/12/20
 */

#ifndef FUCNTIONAL_REDUCTION_HPP
#define FUCNTIONAL_REDUCTION_HPP

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace std;
using namespace mockturtle;

namespace alice {

class reduction_command : public command {
 public:
  explicit reduction_command(const environment::ptr& env)
      : command(env, "functional reduction : using AIG as default") {
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
    if (is_set("mig")) {
      auto mig = store<mig_network>().current();
      functional_reduction(mig);
      mig = cleanup_dangling(mig);
      phyLS::print_stats(mig);
      store<mig_network>().extend();
      store<mig_network>().current() = mig;
    } else if (is_set("xmg")) {
      auto xmg = store<xmg_network>().current();
      functional_reduction(xmg);
      xmg = cleanup_dangling(xmg);
      phyLS::print_stats(xmg);
      store<xmg_network>().extend();
      store<xmg_network>().current() = xmg;
    } else if (is_set("xag")) {
      auto xag = store<xag_network>().current();
      functional_reduction(xag);
      xag = cleanup_dangling(xag);
      phyLS::print_stats(xag);
      store<xag_network>().extend();
      store<xag_network>().current() = xag;
    } else {
      auto aig = store<aig_network>().current();
      functional_reduction(aig);
      aig = cleanup_dangling(aig);
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
};

ALICE_ADD_COMMAND(reduction, "Logic synthesis")

}  // namespace alice

#endif