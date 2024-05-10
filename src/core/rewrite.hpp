#pragma once

#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/akers.hpp>
#include <mockturtle/algorithms/node_resynthesis/exact.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/properties/mccost.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/utils/cost_functions.hpp>
#include <mockturtle/views/fanout_view.hpp>

using namespace percy;
using namespace mockturtle;

namespace phyLS
{
template <class Ntk>
void aig_rewrite(Ntk& ntk) {
  // please learn the algorithm of "mockturtle/algorithms/cut_rewriting.hpp"
}
}