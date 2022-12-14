/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file write_dot.hpp
 *
 * @brief write dot files
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef WRITE_DOT_HPP
#define WRITE_DOT_HPP

#include <mockturtle/mockturtle.hpp>
#include <mockturtle/io/write_dot.hpp>

namespace alice
{

  class write_dot_command : public command
  {
  public:
    explicit write_dot_command(const environment::ptr &env) : command(env, "write dot file")
    {
      add_flag("--xmg_network,-x", "write xmg_network into dot files");
      add_flag("--aig_network,-a", "write aig_network into dot files");
      add_flag("--mig_network,-m", "write mig_network into dot files");
      add_flag("--klut_network,-l", "write klut_network into dot files");
      add_option("--filename, -f", filename, "The path to store dot file, default: /tmp/test.dot");
    }

  protected:
    void execute()
    {
      if (is_set("xmg_network"))
      {
        xmg_network xmg = store<xmg_network>().current();

        write_dot(xmg, filename, gate_dot_drawer<xmg_network>{});
      }
      else if (is_set("aig_network"))
      {
        aig_network aig = store<aig_network>().current();

        write_dot(aig, filename, gate_dot_drawer<aig_network>{});
      }
      else if (is_set("mig_network"))
      {
        mig_network mig = store<mig_network>().current();

        write_dot(mig, filename, gate_dot_drawer<mig_network>{});
      }
      else if (is_set("klut_network"))
      {
        klut_network klut = store<klut_network>().current();

        write_dot(klut, filename, gate_dot_drawer<klut_network>{});
      }
      else
      {
        assert(false && " at least one store should be specified ");
      }
    }

  private:
    std::string filename = "/tmp/test.dot";
  };

  ALICE_ADD_COMMAND(write_dot, "I/O")

}

#endif
