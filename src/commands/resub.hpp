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

#include "../core/resub.hpp"
#include "../core/misc.hpp"

using namespace std;

namespace alice {
class resub_command : public command {
 public:
  explicit resub_command(const environment::ptr& env)
      : command(
            env,
            "performs technology-independent restructuring [default = AIG]") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    if (store<aig_network>().size() == 0u)
      std::cerr << "Error: Empty AIG network\n";
    else {
      auto aig = store<aig_network>().current();
      phyLS::aig_resub(aig);
      phyLS::print_stats(aig);
      store<aig_network>().extend();
      store<aig_network>().current() = aig;
    }
  }
};

ALICE_ADD_COMMAND(resub, "Synthesis")

}  // namespace alice

#endif
