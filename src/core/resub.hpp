#pragma once

#include <kitty/static_truth_table.hpp>
#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/algorithms/sim_resub.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>

using namespace percy;
using namespace mockturtle;

namespace phyLS
{
template <class Ntk>
void aig_resub(Ntk& ntk) {
  // please learn the algorithm of "mockturtle/algorithms/aig_resub.hpp"
}
}