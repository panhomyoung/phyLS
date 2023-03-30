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

class fr_command : public command {
 public:
  explicit fr_command(const environment::ptr& env)
      : command(env, "functional reduction [default = AIG]") {
    add_flag("--mig, -m", "functional reduction for MIG");
    add_flag("--xag, -g", "functional reduction for XAG");
    add_flag("--xmg, -x", "functional reduction for XMG");
    add_option("--tfi_node, -n", max_tfi_node,
               "Maximum number of nodes in the TFI to be compared");
    add_flag("--saturation, -s",
             "repeat until no further improvement can be found");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    functional_reduction_params ps;
    ps.max_TFI_nodes = max_tfi_node;
    if (is_set("saturation")) ps.saturation = true;

    if (is_set("verbose")) {
      ps.verbose = true;
      ps.progress = true;
    }

    begin = clock();
    if (is_set("mig")) {
      auto mig = store<mig_network>().current();
      functional_reduction(mig, ps);
      mig = cleanup_dangling(mig);
      phyLS::print_stats(mig);
      store<mig_network>().extend();
      store<mig_network>().current() = mig;
    } else if (is_set("xmg")) {
      auto xmg = store<xmg_network>().current();
      functional_reduction(xmg, ps);
      xmg = cleanup_dangling(xmg);
      phyLS::print_stats(xmg);
      store<xmg_network>().extend();
      store<xmg_network>().current() = xmg;
    } else if (is_set("xag")) {
      auto xag = store<xag_network>().current();
      functional_reduction(xag, ps);
      xag = cleanup_dangling(xag);
      phyLS::print_stats(xag);
      store<xag_network>().extend();
      store<xag_network>().current() = xag;
    } else {
      auto aig = store<aig_network>().current();
      functional_reduction(aig, ps);
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
  int max_tfi_node = 500;
};

ALICE_ADD_COMMAND(fr, "Synthesis")

}  // namespace alice

#endif