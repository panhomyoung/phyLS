#pragma once

#include <algorithm>
#include <mockturtle/algorithms/aig_balancing.hpp>
#include <mockturtle/algorithms/balancing.hpp>
#include <mockturtle/algorithms/balancing/sop_balancing.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <vector>

using namespace percy;
using namespace mockturtle;

namespace phyLS
{
template <class Ntk>
void aig_balancing(Ntk& ntk) {
    // please learn the algorithm of "mockturtle/algorithms/aig_balancing.hpp"
}

}