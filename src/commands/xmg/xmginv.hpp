/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file xmginv.hpp
 *
 * @brief Inversion optimization of xmg
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef XMGINV_HPP
#define XMGINV_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/utils/stopwatch.hpp>

#include "../../core/xmginv.hpp"

namespace alice {

class xmginv_command : public command {
 public:
  explicit xmginv_command(const environment::ptr& env)
      : command(env, "inversion optimization of xmg") {
    add_flag("--verbose, -v", "print the information");
  }

  rules validity_rules() const { return {has_store_element<xmg_network>(env)}; }

 protected:
  void execute() {
    /* derive some XMG */
    xmg_network xmg = store<xmg_network>().current();
    xmg_network xmg_opt;

    stopwatch<>::duration time{0};
    call_with_stopwatch(
        time, [&]() { xmg_opt = phyLS::xmg_inv_optimization(xmg); });

    store<xmg_network>().extend();
    store<xmg_network>().current() = xmg_opt;

    phyLS::print_stats(xmg_opt);

    std::cout << fmt::format("[time]: {:5.2f} seconds\n", to_seconds(time));
  }
};

ALICE_ADD_COMMAND(xmginv, "Synthesis")

}  // namespace alice

#endif
