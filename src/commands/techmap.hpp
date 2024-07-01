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

#include <iostream>
#include <mockturtle/algorithms/mapper.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/mig.hpp>
#include <mockturtle/networks/xmg.hpp>
#include <mockturtle/properties/xmgcost.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <string>

#include "../core/properties.hpp"
#include "../core/read_placement_file.hpp"

namespace alice {

class techmap_command : public command {
 public:
  explicit techmap_command(const environment::ptr& env)
      : command(env, "Standard cell mapping [default = AIG]") {
    add_flag("--xmg, -x", "Standard cell mapping for XMG");
    add_flag("--mig, -m", "Standard cell mapping for MIG");
    add_flag("--lut, -l", "Standard cell mapping for k-LUT");
    add_option("--output, -o", filename, "the verilog filename");
    add_option("--cut_limit, -c", cut_limit,
               "Maximum number of cuts for a node");
    add_option("--node_position_pl, -p", pl_filename, "the pl filename");
    add_option("--node_position_def, -d", def_filename, "the def filename");
    add_flag("--area, -a", "Area-only standard cell mapping");
    add_flag("--delay, -e", "Delay-only standard cell mapping");
    add_flag("--wirelength, -w", "Wirelength-only standard cell mapping");
    add_flag("--balance, -b", "Balanced wirelength standard cell mapping");
    add_flag("--verbose, -v", "print the information");
  }

  rules validity_rules() const {
    return {has_store_element<std::vector<mockturtle::gate>>(env)};
  }

 private:
  std::string filename = "techmap.v";
  std::string pl_filename = "";
  std::string def_filename = "";
  uint32_t cut_limit{49u};

 protected:
  void execute() {
    /* derive genlib */
    std::vector<mockturtle::gate> gates =
        store<std::vector<mockturtle::gate>>().current();
    mockturtle::tech_library<5> lib(gates);

    mockturtle::map_params ps;
    mockturtle::map_stats st;
    ps.cut_enumeration_ps.cut_limit = cut_limit;

    if (is_set("area"))
      ps.strategy = map_params::area;
    else if (is_set("delay"))
      ps.strategy = map_params::delay;
    else if (is_set("wirelength"))
      ps.strategy = map_params::wirelength;
    else if (is_set("balance"))
      ps.strategy = map_params::balance;
    else
      ps.strategy = map_params::def;
    if (is_set("verbose")) ps.verbose = true;

    stopwatch<>::duration time{0};
    call_with_stopwatch(time, [&]() {
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
      } else if (is_set("mig")) {
        if (store<mig_network>().size() == 0u) {
          std::cerr << "[e] no MIG in the store\n";
        } else {
          auto mig = store<mig_network>().current();

          auto res = mockturtle::map(mig, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
              "Mapped MIG into #gates = {} area = {:.2f} delay = {:.2f}\n",
              res.num_gates(), st.area, st.delay);
        }
      } else if (is_set("lut")) {
        if (store<klut_network>().size() == 0u) {
          std::cerr << "[e] no k-LUT in the store\n";
        } else {
          auto lut = store<klut_network>().current();

          auto res = mockturtle::map(lut, lib, ps, &st);

          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }

          std::cout << fmt::format(
              "Mapped k-LUT into #gates = {} area = {:.2f} delay = {:.2f}\n",
              res.num_gates(), st.area, st.delay);
        }
      } else {
        if (store<aig_network>().size() == 0u) {
          std::cerr << "[e] no AIG in the store\n";
        } else {
          auto aig = store<aig_network>().current();
          if (is_set("node_position_def")) {
            std::vector<mockturtle::node_position> np(aig.size());
            phyLS::read_def_file(def_filename, np);
            ps.wirelength_rounds = true;
            auto res = mockturtle::map(aig, lib, np, ps, &st);
            if (is_set("output")) write_verilog_with_binding(res, filename);
            std::cout << fmt::format(
                "Mapped AIG into #gates = {}, area = {:.2f}, delay = {:.2f}, "
                "power = {:.2f}, wirelength = {:.2f}, total_wirelength = "
                "{:.2f}\n",
                res.num_gates(), st.area, st.delay, st.power, st.wirelength,
                st.total_wirelength);
          } else {
            auto res = mockturtle::map(aig, lib, ps, &st);
            if (is_set("output")) write_verilog_with_binding_new(res, filename);
            std::cout << fmt::format(
                "Mapped AIG into #gates = {}, area = {:.2f}, delay = {:.2f}, "
                "power = {:.2f}\n",
                res.num_gates(), st.area, st.delay, st.power);
          }
        }
      }
    });
    if (is_set("verbose")) st.report();
    std::cout << fmt::format("[CPU time]: {:5.3f} seconds\n", to_seconds(time));
  }
};

ALICE_ADD_COMMAND(techmap, "Mapping")

}  // namespace alice

#endif
