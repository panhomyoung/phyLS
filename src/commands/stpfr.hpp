#ifndef STPFR_HPP
#define STPFR_HPP

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/functional_reduction.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/networks/aig.hpp>

#include "../core/stp_functional_reduction.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {

class stpfr_command : public command {
 public:
  explicit stpfr_command(const environment::ptr& env)
      : command(env, "STP-based functional reduction [default = AIG]") {
    add_option("--tfi_node, -n", max_tfi_node,
               "Maximum number of nodes in the TFI to be compared");
    add_option("--filename, -f", filename, "pre-generated patterns file");
    add_option("--pattern, -p", pattern, "pre-generated equivalent class file");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    phyLS::stp_functional_reduction_params ps;
    if (is_set("--filename")) ps.pattern_filename = filename;
    if (is_set("--tfi_node")) ps.max_TFI_nodes = max_tfi_node;
    if (is_set("--pattern")) ps.equi_classes = pattern;
    if (is_set("--verbose")) {
      ps.verbose = true;
    }

    begin = clock();

    auto aig = store<aig_network>().current();
    phyLS::stp_functional_reduction(aig, ps);
    aig = cleanup_dangling(aig);
    phyLS::print_stats(aig);
    store<aig_network>().extend();
    store<aig_network>().current() = aig;

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  int max_tfi_node;
  string filename;
  string pattern;
};

ALICE_ADD_COMMAND(stpfr, "Synthesis")

}  // namespace alice

#endif