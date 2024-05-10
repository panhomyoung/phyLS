/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file aig_balancing.hpp
 *
 * @brief transforms the current network into a well-balanced AIG
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef BALANCE_HPP
#define BALANCE_HPP

#include "../core/misc.hpp"
#include "../core/balance.hpp"

using namespace std;
using namespace mockturtle;

namespace alice {

class balance_command : public command {
 public:
  explicit balance_command(const environment::ptr& env)
      : command(env,
                "transforms the current network into a well-balanced AIG") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    if (store<aig_network>().size() == 0u)
      std::cerr << "Error: Empty AIG network\n";
    else {
      auto aig = store<aig_network>().current();
      phyLS::aig_balancing(aig);
      phyLS::print_stats(aig);
      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }
  }
};

ALICE_ADD_COMMAND(balance, "Synthesis")

}  // namespace alice

#endif