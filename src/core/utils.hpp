/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file utils.hpp
 *
 * @brief TODO
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/xmg.hpp>

using namespace mockturtle;

namespace phyLS {

std::array<xmg_network::signal, 3> get_children(xmg_network const& xmg,
                                                xmg_network::node const& n) {
  std::array<xmg_network::signal, 3> children;
  xmg.foreach_fanin(n, [&children](auto const& f, auto i) { children[i] = f; });
  std::sort(
      children.begin(), children.end(),
      [&](auto const& c1, auto const& c2) { return c1.index < c2.index; });
  return children;
}

std::array<xag_network::signal, 2> get_xag_children(
    xag_network const& xag, xag_network::node const& n) {
  std::array<xag_network::signal, 2> children;
  xag.foreach_fanin(n, [&children](auto const& f, auto i) { children[i] = f; });
  std::sort(
      children.begin(), children.end(),
      [&](auto const& c1, auto const& c2) { return c1.index < c2.index; });
  return children;
}

void print_children(std::array<xmg_network::signal, 3> const& children) {
  auto i = 0u;
  for (auto child : children) {
    std::cout << "children " << i << " is " << child.index << " complemented ? "
              << child.complement << std::endl;
    i++;
  }
}

}  // namespace phyLS

#endif
