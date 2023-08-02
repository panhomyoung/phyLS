/* phyLS: Advanced Logic Synthesis and Optimization tool
 * Copyright (C) 2019- Ningbo University, Ningbo, China */

/**
 * @file cutrw.hpp
 *
 * @brief cut rewriting
 *
 * @author Zhufei Chu
 * @since  0.1
 */

#ifndef CUTRW_HPP
#define CUTRW_HPP

#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>
#include <mockturtle/mockturtle.hpp>

#include "../core/misc.hpp"
#include "../networks/aoig/xag_lut_npn.hpp"
#include "../networks/stp/stp_npn.hpp"

using namespace std;

namespace alice {

class cutrw_command : public command {
 public:
  explicit cutrw_command(const environment::ptr& env)
      : command(env, "Performs cut rewriting") {
    add_flag("--xag_npn_lut,-g", "cut rewriting based on xag_npn_lut");
    add_flag("--xag_npn,-p", "cut rewriting based on xag_npn");
    add_flag("--klut_npn,-l", "cut rewriting based on klut_npn");
    add_flag("--compatibility_graph, -c", "In-place cut rewriting");
  }

  void execute() {
    clock_t begin, end;
    double totalTime = 0.0;
    begin = clock();
    /* parameters */
    if (is_set("xag_npn_lut")) {
      xag_network xag = store<xag_network>().current();

      phyLS::print_stats(xag);

      xag_npn_lut_resynthesis resyn;
      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size = 4u;
      xag = cut_rewriting(xag, resyn, ps);

      xag = cleanup_dangling(xag);

      // bidecomposition refactoring
      bidecomposition_resynthesis<xag_network> resyn2;
      refactoring(xag, resyn2);

      xag = cleanup_dangling(xag);

      phyLS::print_stats(xag);

      store<xag_network>().extend();
      store<xag_network>().current() = cleanup_dangling(xag);
    } else if (is_set("xag_npn")) {
      begin = clock();
      xag_network xag = store<xag_network>().current();

      phyLS::print_stats(xag);

      xag_npn_resynthesis<xag_network> resyn;
      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size = 4u;
      ps.min_cand_cut_size = 2;
      ps.min_cand_cut_size_override = 3;
      xag = cut_rewriting(xag, resyn, ps);
      xag = cleanup_dangling(xag);

      phyLS::print_stats(xag);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

      store<xag_network>().extend();
      store<xag_network>().current() = cleanup_dangling(xag);
    } else if (is_set("klut_npn")) {
      klut_network klut = store<klut_network>().current();
      phyLS::print_stats(klut);

      stp_npn_resynthesis resyn;
      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size = 4u;
      ps.allow_zero_gain = false;
      if (is_set("compatibility_graph"))
          cut_rewriting_with_compatibility_graph(klut, resyn);
        else
          klut = cut_rewriting(klut, resyn, ps);
      klut = cleanup_dangling(klut);

      phyLS::print_stats(klut);
      store<klut_network>().extend();
      store<klut_network>().current() = cleanup_dangling(klut);
    } else {
      xmg_network xmg = store<xmg_network>().current();

      phyLS::print_stats(xmg);

      xmg_npn_resynthesis resyn;
      cut_rewriting_params ps;
      ps.cut_enumeration_ps.cut_size = 4u;
      xmg = cut_rewriting(xmg, resyn, ps);
      xmg = cleanup_dangling(xmg);

      phyLS::print_stats(xmg);

      store<xmg_network>().extend();
      store<xmg_network>().current() = cleanup_dangling(xmg);
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[Total CPU time]   : " << setprecision(3) << totalTime << " s"
         << endl;
  }

 private:
  bool verbose = false;
};

ALICE_ADD_COMMAND(cutrw, "Synthesis")

}  // namespace alice

#endif
