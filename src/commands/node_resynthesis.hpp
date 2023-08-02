/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file node_resynthesis.hpp
 *
 * @brief Node resynthesis
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef NODE_RESYNTHESIS_HPP
#define NODE_RESYNTHESIS_HPP

#include <time.h>

#include <algorithm>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <mockturtle/algorithms/node_resynthesis.hpp>
#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/direct.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg_npn.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

#include "../core/misc.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {
class resyn_command : public command {
 public:
  explicit resyn_command(const environment::ptr& env)
      : command(env,
                "performs technology-independent restructuring [default = MIG]") {
    add_flag("--xmg, -x", "Resubstitution for XMG");
    add_flag("--xag, -g", "Resubstitution for XAG");
    add_flag("--aig, -a", "Resubstitution for AIG");
    add_flag("--direct, -d", "Node resynthesis with direct synthesis");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;
    if (store<klut_network>().size() == 0u)
      std::cerr << "Error: Empty k-LUT network\n";
    else {
      auto klut = store<klut_network>().current();
      if (is_set("xmg")) {
        begin = clock();
        if (is_set("direct")) {
          direct_resynthesis<xmg_network> xmg_resyn;
          const auto xmg = node_resynthesis<xmg_network>(klut, xmg_resyn);
          store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling(xmg);
          phyLS::print_stats(xmg);
        } else {
          xmg_npn_resynthesis resyn;
          const auto xmg = node_resynthesis<xmg_network>(klut, resyn);
          store<xmg_network>().extend();
          store<xmg_network>().current() = cleanup_dangling(xmg);
          phyLS::print_stats(xmg);
        }
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      } else if (is_set("xag")) {
        begin = clock();
        xag_npn_resynthesis<xag_network> resyn;
        const auto xag = node_resynthesis<xag_network>(klut, resyn);
        store<xag_network>().extend();
        store<xag_network>().current() = cleanup_dangling(xag);
        phyLS::print_stats(xag);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      } else if (is_set("aig")) {
        begin = clock();
        exact_resynthesis_params ps;
        ps.cache = std::make_shared<exact_resynthesis_params::cache_map_t>();
        exact_aig_resynthesis<aig_network> exact_resyn(false, ps);
        node_resynthesis_stats nrst;
        dsd_resynthesis<aig_network, decltype(exact_resyn)> resyn(exact_resyn);
        const auto aig = node_resynthesis<aig_network>(klut, resyn, {}, &nrst);
        store<aig_network>().extend();
        store<aig_network>().current() = cleanup_dangling(aig);
        phyLS::print_stats(aig);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      } else {
        begin = clock();
        if (is_set("direct")) {
          direct_resynthesis<mig_network> mig_resyn;
          const auto mig = node_resynthesis<mig_network>(klut, mig_resyn);
          store<mig_network>().extend();
          store<mig_network>().current() = cleanup_dangling(mig);
          phyLS::print_stats(mig);
        } else {
          mig_npn_resynthesis resyn;
          const auto mig = node_resynthesis<mig_network>(klut, resyn);
          store<mig_network>().extend();
          store<mig_network>().current() = cleanup_dangling(mig);
          phyLS::print_stats(mig);
        }
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      }

      cout.setf(ios::fixed);
      cout << "[CPU time]   " << setprecision(3) << totalTime << " s" << endl;
    }
  }
};

ALICE_ADD_COMMAND(resyn, "Synthesis")

}  // namespace alice

#endif
