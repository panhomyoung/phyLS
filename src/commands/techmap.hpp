/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

 /**
  * @file techmap.hpp
  *
  * @brief Standard cell mapping
  *
  * @author Homyoung
  * @since  2022/12/14
  */

#ifndef TECHMAP_HPP
#define TECHMAP_HPP

#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/xmgcost.hpp>
#include <mockturtle/utils/tech_library.hpp>

#include "../core/properties.hpp"

namespace alice {

  class techmap_command: public command {
  public:
    explicit techmap_command(const environment::ptr& env)
      : command(env, "Standard cell mapping : using AIG as default") {
      add_flag("--xmg, -x", "Standard cell mapping for XMG");
      add_flag("--mig, -m", "Standard cell mapping for MIG");
      add_flag("--lut, -l", "Standard cell mapping for k-LUT");
      add_option("--output, -o", filename, "the verilog filename");
      add_flag("--verbose, -v", "print the information");
    }

    rules validity_rules() const {
      return { has_store_element<std::vector<mockturtle::gate>>(env) };
    }

  private:
    std::string filename = "techmap.v";

  protected:
    void execute() {
      /* derive genlib */
      std::vector<mockturtle::gate> gates =
        store<std::vector<mockturtle::gate>>().current();

      mockturtle::tech_library<5> lib(gates);
      mockturtle::map_params ps;
      mockturtle::map_stats st;

      if (is_set("xmg")) {
        if (store<xmg_network>().size() == 0u)
          std::cerr << "[e] no XMG in the store\n";
        else {
          auto xmg = store<xmg_network>().current();
          xmg_gate_stats stats;
          xmg_profile_gates(xmg, stats);
          std::cout << "[i] ";
          stats.report();

          phyLS::xmg_critical_path_stats critical_stats;
          phyLS::xmg_critical_path_profile_gates(xmg, critical_stats);
          std::cout << "[i] ";
          critical_stats.report();

          auto res = mockturtle::map(xmg, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
            "[i] Mapped XMG into #gates = {} area = {:.2f} delay = {:.2f}\n",
            res.num_gates(), st.area, st.delay);
        }
      }
      else if (is_set("mig")) {
        if (store<mig_network>().size() == 0u) {
          std::cerr << "[e] no MIG in the store\n";
        }
        else {
          auto mig = store<mig_network>().current();

          auto res = mockturtle::map(mig, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
            "Mapped MIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
            res.num_gates(), st.area, st.delay);
        }
      }
      else if (is_set("lut")) {
        if (store<klut_network>().size() == 0u) {
          std::cerr << "[e] no k-LUT in the store\n";
        }
        else {
          auto lut = store<klut_network>().current();

          auto res = mockturtle::map(lut, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
            "Mapped k-LUT into #gates = {} area = {:.2f} delay = {:.2f}\n",
            res.num_gates(), st.area, st.delay);
        }
      }
      else {
        if (store<aig_network>().size() == 0u) {
          std::cerr << "[e] no AIG in the store\n";
        }
        else {
          auto aig = store<aig_network>().current();

          auto res = mockturtle::map(aig, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
            "Mapped AIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
            res.num_gates(), st.area, st.delay);
        }
      }
    }
  };

  ALICE_ADD_COMMAND(techmap, "Mapping")

}  // namespace alice

#endif
