/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file properties.hpp
 *
 * @brief TODO
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef PROPERTIES_HPP
#define PROPERTIES_HPP

#include <fmt/format.h>

#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/views/depth_view.hpp>

using namespace mockturtle;

namespace phyLS {
struct xmg_critical_path_stats {
  uint32_t xor3{0};
  uint32_t xor2{0};
  uint32_t maj{0};
  uint32_t and_or{0};

  void report() const {
    fmt::print("On critical path: XOR3: {}, XOR2: {}, MAJ: {}, AND/OR: {}\n",
               xor3, xor2, maj, and_or);
  }
};

void xmg_critical_path_profile_gates(xmg_network const& xmg,
                                     xmg_critical_path_stats& stats);

}  // namespace phyLS

#endif
