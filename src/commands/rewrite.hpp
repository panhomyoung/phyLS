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

#include "../core/rewrite.hpp"
#include "../core/misc.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {
class rewrite_command : public command {
 public:
  explicit rewrite_command(const environment::ptr& env)
      : command(env, "on-the-fly DAG-aware logic rewriting [default = AIG]") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
      if (store<aig_network>().size() == 0u)
        std::cerr << "Error: Empty AIG network\n";
      else {
        auto aig = store<aig_network>().current();
        phyLS::aig_rewrite(aig);
        phyLS::print_stats(aig);
        store<aig_network>().extend();
        store<aig_network>().current() = aig;
      }
  }
};

ALICE_ADD_COMMAND(rewrite, "Synthesis")

}  // namespace alice

#endif
