#ifndef XMG_EXTRACT_HPP
#define XMG_EXTRACT_HPP

#include <mockturtle/algorithms/circuit_validator.hpp>
#include <mockturtle/views/topo_view.hpp>

#include "utils.hpp"

using namespace mockturtle;

namespace phyLS
{
  
  std::vector<unsigned> xmg_extract( xmg_network const& xmg )
  {
    std::vector<unsigned> xors;
    topo_view topo{ xmg };
    circuit_validator v( xmg );

    int count = 0;
    
    topo.foreach_node( [&]( auto n ) {
    if( xmg.is_xor3( n ) )
    {
      auto children = get_children( xmg, n );

      //print_children( children );

      if( xmg.get_node( children[0] ) == 0 
          && !xmg.is_pi( xmg.get_node( children[1] ) ) 
          && !xmg.is_pi( xmg.get_node( children[2] ) ) )
      { 
        auto result1 = v.validate( children[1], children[2] );
        auto result2 = v.validate( children[1], !children[2] );

        if( result1 == std::nullopt || result2 == std::nullopt )
        {
          //std::cout << "UNKNOWN status\n";
          return;
        }
        
        if( result1 || result2 )
        {
          if( *result1 )
          {
            std::cout << "XOR node " << n  << " can be removed as constant 0\n";
            xors.push_back( n * 2 );
            count++;
          }
          else if( *result2 )
          {
            std::cout << "XOR node " << n  << " can be removed as constant 1\n";
            xors.push_back( n * 2 + 1 );
            count++;
          }
        }
      }
    }
    } );

    std::cout << "[xmgrw -u] Find " << count << " XOR constant nodes\n";
    return xors;
  }
}

#endif
