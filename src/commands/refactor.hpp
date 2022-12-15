/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file aig_balancing.hpp
 *
 * @brief performs technology-independent refactoring
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef REFACTOR_HPP
#define REFACTOR_HPP

#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>
#include <mockturtle/algorithms/node_resynthesis/mig_npn.hpp>
#include <mockturtle/algorithms/refactoring.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/traits.hpp>

#include "../core/misc.hpp"

using namespace mockturtle;
using namespace std;

namespace alice {

class refactor_command : public command {
 public:
  explicit refactor_command(const environment::ptr& env)
      : command(env, "performs technology-independent refactoring") {
    add_flag("--mig, -m", "refactoring for MIG");
    add_flag("--xag, -g", "refactoring for XAG");
    add_flag("--xmg, -x", "refactoring for XMG");
    add_flag("--akers, -a", "Refactoring with Akers synthesis for MIG");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    if (is_set("mig")) {
      if (store<mig_network>().size() == 0u)
        std::cerr << "Error: Empty MIG network\n";
      else {
        auto mig = store<mig_network>().current();
        begin = clock();
        if (is_set("akers")) {
          akers_resynthesis<mig_network> resyn;
          refactoring_params ps;
          ps.max_pis = 4u;
          refactoring(mig, resyn, ps);
        } else {
          mig_npn_resynthesis resyn;
          refactoring_params ps;
          ps.max_pis = 4u;
          refactoring(mig, resyn, ps);
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
        bidecomposition_resynthesis<xag_network> resyn;
        refactoring_params ps;
        ps.max_pis = 4u;
        refactoring(xag, resyn, ps);
        xag = cleanup_dangling(xag);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(xag);
        store<xag_network>().extend();
        store<xag_network>().current() = xag;
      }
    } else if (is_set("xmg")) {
      if (store<xmg_network>().size() == 0u)
        std::cerr << "Error: Empty XMG network\n";
      else {
        auto xmg = store<xmg_network>().current();
        begin = clock();
        xmg_npn_resynthesis resyn;
        refactoring_params ps;
        ps.max_pis = 4u;
        refactoring(xmg, resyn, ps);
        xmg = cleanup_dangling(xmg);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(xmg);
        store<xmg_network>().extend();
        store<xmg_network>().current() = xmg;
      }
    } else {
      if (store<aig_network>().size() == 0u)
        std::cerr << "Error: Empty AIG network\n";
      else {
        auto aig = store<aig_network>().current();
        begin = clock();
        direct_resynthesis<aig_network> aig_resyn;
        refactoring_params ps;
        ps.max_pis = 4u;
        refactoring(aig, aig_resyn, ps);
        aig = cleanup_dangling(aig);
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

 private:
};

ALICE_ADD_COMMAND(refactor, "Logic synthesis")

}  // namespace alice

#endif