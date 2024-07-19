/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file lut_mapping.hpp
 *
 * @brief lut mapping for any input
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef LUT_MAPPING_HPP
#define LUT_MAPPING_HPP

#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/algorithms/satlut_mapping.hpp>
#include <mockturtle/generators/arithmetic.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/networks/sequential.hpp>
#include <mockturtle/traits.hpp>
#include <mockturtle/views/mapping_view.hpp>

using namespace mockturtle;
using namespace std;

namespace alice {

class lut_mapping_command : public command {
 public:
  explicit lut_mapping_command(const environment::ptr &env)
      : command(env, "LUT mapping [default = AIG]") {
    add_option("cut_size, -k", cut_size,
               "set the cut size from 2 to 8, default = 4");
    add_flag("--satlut, -s", "satlut mapping");
    add_flag("--xag, -g", "LUT mapping for XAG");
    add_flag("--mig, -m", "LUT mapping for MIG");
    add_flag("--klut, -l", "LUT mapping for k-LUT");
    add_flag("--area, -a", "area-oriented mapping, default = no");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;
    lut_mapping_params ps;
    if (is_set("area")) ps.rounds = 0u;

    if (is_set("mig")) {
      /* derive some MIG */
      assert(store<mig_network>().size() > 0);
      begin = clock();
      mig_network mig = store<mig_network>().current();

      mapping_view<mig_network, true> mapped_mig{mig};
      ps.cut_enumeration_ps.cut_size = cut_size;
      lut_mapping<mapping_view<mig_network, true>, true>(mapped_mig, ps);

      /* collapse into k-LUT network */
      const auto klut = *collapse_mapped_network<klut_network>(mapped_mig);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      store<klut_network>().extend();
      store<klut_network>().current() = klut;
    } else if (is_set("xag")) {
      /* derive some XAG */
      assert(store<xag_network>().size() > 0);
      begin = clock();
      xag_network xag = store<xag_network>().current();

      mapping_view<xag_network, true> mapped_xag{xag};
      ps.cut_enumeration_ps.cut_size = cut_size;
      lut_mapping<mapping_view<xag_network, true>, true>(mapped_xag, ps);

      /* collapse into k-LUT network */
      const auto klut = *collapse_mapped_network<klut_network>(mapped_xag);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      store<klut_network>().extend();
      store<klut_network>().current() = klut;
    } else if (is_set("klut")) {
      /* derive some kLUT */
      assert(store<klut_network>().size() > 0);
      begin = clock();
      klut_network klut = store<klut_network>().current();

      mapping_view<klut_network, true> mapped_klut{klut};
      ps.cut_enumeration_ps.cut_size = cut_size;
      lut_mapping<mapping_view<klut_network, true>, true>(mapped_klut, ps);

      /* collapse into k-LUT network */
      const auto klut_new = *collapse_mapped_network<klut_network>(mapped_klut);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      store<klut_network>().extend();
      store<klut_network>().current() = klut_new;
    } else {
      if (store<aig_network>().size() == 0) {
        assert(false && "Error: Empty AIG network\n");
      }
      /* derive some AIG */
      aig_network aig = store<aig_network>().current();
      begin = clock();
      mapping_view<aig_network, true> mapped_aig{aig};

      /* LUT mapping */
      if (is_set("satlut")) {
        satlut_mapping_params ps;
        ps.cut_enumeration_ps.cut_size = cut_size;
        satlut_mapping<mapping_view<aig_network, true>, true>(mapped_aig, ps);
      } else {
        ps.cut_enumeration_ps.cut_size = cut_size;
        lut_mapping<mapping_view<aig_network, true>, true,
                    cut_enumeration_mf_cut>(mapped_aig, ps);
      }

      /* collapse into k-LUT network */
      const auto klut = *collapse_mapped_network<klut_network>(mapped_aig);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      store<klut_network>().extend();
      store<klut_network>().current() = klut;
    }
    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  unsigned cut_size = 4u;
};

ALICE_ADD_COMMAND(lut_mapping, "Mapping")
}  // namespace alice

#endif
