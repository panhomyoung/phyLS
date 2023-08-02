#pragma once

#include <iostream>
#include <type_traits>
#include <vector>

#include <kitty/operations.hpp>

#include <mockturtle/traits.hpp>
#include <mockturtle/utils/node_map.hpp>
#include <mockturtle/views/topo_view.hpp>

namespace mockturtle
{

template<typename NtkSource, typename NtkDest, typename LeavesIterator>
std::vector<signal<NtkDest>> stp_cleanup_dangling( NtkSource const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end )
{
  (void)end;

  node_map<signal<NtkDest>, NtkSource> old_to_new( ntk );
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );

  /* create inputs in same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  assert( it == end );

  /* foreach node in topological order */
  topo_view topo{ntk};
  topo.foreach_node( [&]( auto node ) {
    if ( ntk.is_constant( node ) || ntk.is_pi( node ) )
      return;

    /* collect children */
    std::vector<signal<NtkDest>> children;
    ntk.foreach_fanin( node, [&]( auto child, auto ) {
      const auto f = old_to_new[child];
      children.push_back( f );
    } );
      
    old_to_new[node] = dest.clone_node( ntk, node, children );
  } );

  /* create outputs in same order */
  std::vector<signal<NtkDest>> fs;
  ntk.foreach_po( [&]( auto po ) {
    const auto f = old_to_new[po];
    fs.push_back( f );
  } );

  return fs;
}


} // namespace mockturtle
