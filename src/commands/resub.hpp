/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file resub.hpp
 *
 * @brief performs technology-independent restructuring
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef RESUB_HPP
#define RESUB_HPP

#include <time.h>

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/mig_resub.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/algorithms/sim_resub.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/algorithms/xag_resub_withDC.hpp>
#include <mockturtle/algorithms/xmg_resub.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

#include "../core/misc.hpp"

using namespace std;

namespace alice {
class resub_command : public command {
 public:
  explicit resub_command(const environment::ptr& env)
      : command(
            env,
            "performs technology-independent restructuring [default = AIG]") {
    add_flag("--xmg, -x", "Resubstitution for XMG");
    add_flag("--mig, -m", "Resubstitution for MIG");
    add_flag("--xag, -g", "Resubstitution for XAG");
    add_flag("--simulation, -s", "Simulation-guided resubstitution for AIG");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime = 0.0;

    if (is_set("xmg")) {
      if (store<xmg_network>().size() == 0u)
        std::cerr << "Error: Empty XMG network\n";
      else {
        auto xmg = store<xmg_network>().current();
        begin = clock();
        using view_t = depth_view<fanout_view<xmg_network>>;
        fanout_view<xmg_network> fanout_view{xmg};
        view_t resub_view{fanout_view};
        xmg_resubstitution(resub_view);
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
        using view_t = depth_view<fanout_view<mig_network>>;
        fanout_view<mig_network> fanout_view{mig};
        view_t resub_view{fanout_view};
        mig_resubstitution(resub_view);
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
        resubstitution_params ps;
        using view_t = depth_view<fanout_view<xag_network>>;
        fanout_view<xag_network> fanout_view{xag};
        view_t resub_view{fanout_view};
        resubstitution_minmc_withDC(resub_view, ps);
        xag = cleanup_dangling(xag);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        phyLS::print_stats(xag);
        store<xag_network>().extend();
        store<xag_network>().current() = xag;
      }
    } else {
      if (store<aig_network>().size() == 0u)
        std::cerr << "Error: Empty AIG network\n";
      else {
        auto aig = store<aig_network>().current();
        if (is_set("simulation")) {
          begin = clock();
          sim_resubstitution(aig);
          aig = cleanup_dangling(aig);
          end = clock();
          totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        } else {
          begin = clock();
          using view_t = depth_view<fanout_view<aig_network>>;
          fanout_view<aig_network> fanout_view{aig};
          view_t resub_view{fanout_view};
          aig_resubstitution(resub_view);
          aig = cleanup_dangling(aig);
          end = clock();
          totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        }
        phyLS::print_stats(aig);
        store<aig_network>().extend();
        store<aig_network>().current() = aig;
      }
    }

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }
};

ALICE_ADD_COMMAND(resub, "Synthesis")

}  // namespace alice

#endif
