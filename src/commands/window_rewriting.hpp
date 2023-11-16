/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file window_rewriting.hpp
 *
 * @brief Window rewriting
 *
 * @author Homyoung
 * @since  2023/09/27
 */

#ifndef WINDOW_REWRITING_HPP
#define WINDOW_REWRITING_HPP

#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/experimental/boolean_optimization.hpp>
#include <mockturtle/algorithms/experimental/sim_resub.hpp>
#include <mockturtle/algorithms/experimental/window_resub.hpp>
#include <mockturtle/algorithms/window_rewriting.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace std;
using namespace mockturtle;
using namespace mockturtle::experimental;

namespace alice {

class wr_command : public command {
 public:
  explicit wr_command(const environment::ptr& env)
      : command(env, "window rewriting") {
    add_option("cut_size, -k", cut_size,
               "set the cut size from 2 to 6, default = 4");
    add_option("num_levels, -l", num_levels,
               "set the window level, default = 5");
    add_flag("--gain, -g", "optimize until there is no gain");
    add_flag("--resub, -r", "window resub");
    add_flag("--mffw, -w", "MFFW rewriting");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    auto aig = store<aig_network>().current();

    window_rewriting_params ps;
    ps.cut_size = cut_size;
    ps.num_levels = num_levels;

    if (is_set("resub")) {
      window_resub_params ps_r;
      if (is_set("gain")) {
        window_aig_enumerative_resub(aig, ps_r);
      } else {
        window_aig_heuristic_resub(aig, ps_r);
      }
      aig = cleanup_dangling(aig);
    } else {
      if (is_set("gain")) {
        uint64_t const size_before{aig.num_gates()};
        uint64_t size_current{};
        do {
          size_current = aig.num_gates();
          window_rewriting_stats win_st;
          window_rewriting(aig, ps, &win_st);
          if (is_set("verbose")) win_st.report();
          aig = cleanup_dangling(aig);
        } while (aig.num_gates() < size_current);
      } else if (is_set("mffw")) {
        window_rewriting_stats win_st;
        mffw_rewriting(aig, ps, &win_st);
        aig = cleanup_dangling(aig);
      } else {
        window_rewriting_stats win_st;
        window_rewriting(aig, ps, &win_st);
        aig = cleanup_dangling(aig);
      }
    }

    phyLS::print_stats(aig);
    store<aig_network>().extend();
    store<aig_network>().current() = aig;

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  unsigned cut_size = 4u;
  unsigned num_levels = 5u;
};

ALICE_ADD_COMMAND(wr, "Synthesis")

}  // namespace alice

#endif