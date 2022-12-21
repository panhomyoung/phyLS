/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file cut_rewriting.hpp
 *
 * @brief on-the-fly DAG-aware logic rewriting
 *
 * @author Homyoung
 * @since  2022/12/15
 */

#ifndef CUT_REWRITING_HPP
#define CUT_REWRITING_HPP

#include <time.h>

#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_minmc2.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/node_resynthesis/xmg3_npn.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include "../core/misc.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {
class rewrite_command : public command {
 public:
  explicit rewrite_command(const environment::ptr& env)
      : command(env, "on-the-fly DAG-aware logic rewriting [default = AIG]") {
    add_flag("--xmg, -x", "rewriting for XMG");
    add_flag("--mig, -m", "rewriting for MIG");
    add_flag("--xag, -g", "rewriting for XAG");
    add_flag("--klut, -l", "rewriting for k-LUT");
    add_flag("--akers, -a", "Cut rewriting with Akers synthesis for MIG");
    add_flag("--compatibility_graph, -c", "In-place cut rewriting");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    if (is_set("xmg")) {
      if (store<xmg_network>().size() == 0u)
        std::cerr << "Error: Empty XMG network\n";
      else {
        auto xmg = store<xmg_network>().current();
        begin = clock();
        xmg_npn_resynthesis resyn;
        cut_rewriting_params ps;
        ps.cut_enumeration_ps.cut_size = 4u;
        if (is_set("compatibility_graph"))
          cut_rewriting_with_compatibility_graph(xmg, resyn, ps);
        else
          xmg = cut_rewriting(xmg, resyn, ps);
        xmg = cleanup_dangling(xmg);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(xmg);
        store<xmg_network>().extend();
        store<xmg_network>().current() = xmg;
      }
    } else if (is_set("mig")) {
      if (store<mig_network>().size() == 0u)
        std::cerr << "Error: Empty MIG network\n";
      else {
        auto mig = store<mig_network>().current();
        begin = clock();
        if (is_set("akers")) {
          akers_resynthesis<mig_network> resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          if (is_set("compatibility_graph"))
            cut_rewriting_with_compatibility_graph(mig, resyn, ps);
          else
            mig = cut_rewriting(mig, resyn, ps);
        } else {
          mig_npn_resynthesis resyn;
          cut_rewriting_params ps;
          ps.cut_enumeration_ps.cut_size = 4u;
          if (is_set("compatibility_graph"))
            cut_rewriting_with_compatibility_graph(mig, resyn, ps);
          else
            mig = cut_rewriting(mig, resyn, ps);
        }
        mig = cleanup_dangling(mig);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(mig);
        store<mig_network>().extend();
        store<mig_network>().current() = mig;
      }
    } else if (is_set("xag")) {
      if (store<xag_network>().size() == 0u)
        std::cerr << "Error: Empty XAG network\n";
      else {
        auto xag = store<xag_network>().current();
        begin = clock();
        xag_npn_resynthesis<xag_network> resyn;
        cut_rewriting_params ps;
        ps.cut_enumeration_ps.cut_size = 4u;
        ps.min_cand_cut_size = 2;
        ps.min_cand_cut_size_override = 3;
        if (is_set("compatibility_graph")) {
          cut_rewriting_with_compatibility_graph(xag, resyn, ps, nullptr,
                                                 mc_cost<xag_network>());
        } else {
          xag = cut_rewriting(xag, resyn, ps);
        }
        xag = cleanup_dangling(xag);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(xag);
        store<xag_network>().extend();
        store<xag_network>().current() = xag;
      }
    } else if (is_set("klut")) {
      if (store<klut_network>().size() == 0u)
        std::cerr << "Error: Empty k-LUT network\n";
      else {
        auto klut = store<klut_network>().current();
        begin = clock();
        exact_resynthesis resyn(3u);
        if (is_set("compatibility_graph"))
          cut_rewriting_with_compatibility_graph(klut, resyn);
        else
          klut = cut_rewriting(klut, resyn);
        klut = cleanup_dangling(klut);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(klut);
        store<klut_network>().extend();
        store<klut_network>().current() = klut;
      }
    } else {
      if (store<aig_network>().size() == 0u)
        std::cerr << "Error: Empty AIG network\n";
      else {
        auto aig = store<aig_network>().current();
        begin = clock();
        xag_npn_resynthesis<aig_network> resyn;
        cut_rewriting_params ps;
        ps.cut_enumeration_ps.cut_size = 4;
        aig = cut_rewriting(aig, resyn, ps);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(aig);
        store<aig_network>().extend();
        store<aig_network>().current() = aig;
      }
    }

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }
};

ALICE_ADD_COMMAND(rewrite, "Synthesis")

}  // namespace alice

#endif
