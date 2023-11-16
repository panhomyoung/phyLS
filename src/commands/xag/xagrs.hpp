/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file xagrs.hpp
 *
 * @brief XAG resubsitution
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef XAGRS_HPP
#define XAGRS_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/networks/xag.hpp>

#include "../../core/misc.hpp"

namespace alice
{

  class xagrs_command : public command
  {
    public:
      explicit xagrs_command( const environment::ptr& env ) : command( env, "Performs XAG resubsitution" )
      {
        add_flag( "-v,--verbose", ps.verbose, "show statistics" );
      }
      
      rules validity_rules() const
      {
        return { has_store_element<xag_network>( env ) };
      }

    protected:
      void execute()
      {
        /* derive some XAG */
         xag_network xag = store<xag_network>().current();

         default_resubstitution( xag, ps, &st );
         xag = cleanup_dangling( xag );
         
         std::cout << "[xagrs] "; 
         phyLS::print_stats( xag ); 

         store<xag_network>().extend(); 
         store<xag_network>().current() = xag;
      }
    
    private:
      resubstitution_params ps;
      resubstitution_stats  st;
  };
  
  ALICE_ADD_COMMAND( xagrs, "Synthesis" )


}

#endif
