/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2024 */

/**
 * @file to_npz.hpp
 *
 * @brief transforms the current network into adjacency matrix(.npz)
 *
 * @author Homyoung
 * @since  2024/03/01
 */

#ifndef TO_NPZ_HPP
#define TO_NPZ_HPP

#include <algorithm>
#include <mockturtle/networks/aig.hpp>
#include <vector>

#include "../core/tonpz.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {

class write_npz_command : public command {
 public:
  explicit write_npz_command(const environment::ptr& env)
      : command(env, "write npz file, default: AIG") {
    add_flag("--xmg_network,-x", "write xmg_network into dot files");
    add_flag("--mig_network,-m", "write mig_network into dot files");
    add_flag("--klut_network,-l", "write klut_network into dot files");
    add_option("--csv, -c", filename_csv,
               "The path to store csv file, default: ./placement/test.csv");
    add_option("--def, -d", filename_def,
               "The path to store def file, default: "
               "./placement/mfloorplan.def & ./placement/floorplan.def");
  }

 protected:
  void execute() {
    if (is_set("xmg_network")) {
      xmg_network xmg = store<xmg_network>().current();
    } else if (is_set("mig_network")) {
      mig_network mig = store<mig_network>().current();
    } else if (is_set("klut_network")) {
      klut_network klut = store<klut_network>().current();
    } else {
      aig_network aig = store<aig_network>().current();
      if (is_set("csv")) {
        phyLS::write_npz(aig, filename_csv);
      } else if (is_set("def")) {
        phyLS::write_def(aig, filename_def, filename_mdef);
      } else {
        assert(false && "At least one filename should be specified. ");
      }
    }
  }

 private:
  std::string filename_csv = "./placement/test.csv";
  std::string filename_def = "./placement/ploorplan.def";
  std::string filename_mdef = "./placement/mfloorplan.def";
};

ALICE_ADD_COMMAND(write_npz, "I/O")

}  // namespace alice

#endif