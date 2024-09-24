/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file exactlut.hpp
 *
 * @brief Exact LUT mapping with STP based exact synthesis
 *
 * @author Homyoung
 * @since  2024/01/16
 */

#ifndef EXACTLUT_COMMAND_HPP
#define EXACTLUT_COMMAND_HPP

#include <alice/alice.hpp>
#include <mockturtle/algorithms/collapse_mapped.hpp>
#include <mockturtle/algorithms/lut_mapping.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/klut.hpp>
#include <mockturtle/views/mapping_view.hpp>
#include <percy/percy.hpp>

#include "../../core/exact/exact_lut_mapping.hpp"
#include "../../core/misc.hpp"

using namespace std;
using namespace percy;
using namespace mockturtle;
using kitty::dynamic_truth_table;

namespace alice {
class elm_command : public command {
 public:
  explicit elm_command(const environment::ptr& env)
      : command(env, "Performs Exact LUT mapping") {
    add_option("cut_size, -k", cut_size,
               "the number of LUT inputs form 2 to 8, default = 4");
    add_flag("--cec, -c,", "apply equivalence checking");
    add_flag("--best_result, -b,", "keep best result");
    add_flag("--decomposition, -d,", "LUT mapping by decomposition");
    add_flag("--mix, -m,", "LUT mapping by mix approach");
    add_flag("--verbose, -v", "verbose results, default = true");
  }

 protected:
  void execute() {
    mockturtle::klut_network klut = store<mockturtle::klut_network>().current();
    mockturtle::klut_network klut_orig, klut_opt;
    klut_orig = klut;
    phyLS::exact_lut_mapping_params ps;
    ps.cut_size = cut_size;
    stopwatch<>::duration time{0};
    call_with_stopwatch(time, [&]() {
      if (is_set("decomposition")) {
        klut_opt = phyLS::exact_lut_mapping_dec(klut, ps);
      } else if (is_set("mix")) {
        klut_opt = phyLS::exact_lut_mapping_mix(klut, ps);
      } else {
        klut_opt = phyLS::exact_lut_mapping(klut, ps);
      }

      if (is_set("cec")) {
        /* equivalence checking */
        const auto miter_klut = *miter<klut_network>(klut_orig, klut_opt);
        equivalence_checking_stats eq_st;
        const auto result = equivalence_checking(miter_klut, {}, &eq_st);
        assert(*result);
      }
    });
    if (is_set("best_result")) {
      // original LUT mapping results
      mockturtle::aig_network aig = store<mockturtle::aig_network>().current();
      mapping_view<mockturtle::aig_network, true> mapped_aig{aig};
      lut_mapping_params psl;
      psl.cut_enumeration_ps.cut_size = cut_size;
      lut_mapping<mapping_view<mockturtle::aig_network, true>, true,
                  cut_enumeration_mf_cut>(mapped_aig, psl);
      const auto klut_result =
          *collapse_mapped_network<klut_network>(mapped_aig);
      // compare results
      if (klut_opt.num_gates() > klut_result.num_gates())
        klut_opt = klut_result;
    }
    phyLS::print_stats(klut_opt);
    store<mockturtle::klut_network>().extend();
    store<mockturtle::klut_network>().current() = klut_opt;
    std::cout << fmt::format("[CPU time]: {:5.3f} seconds\n", to_seconds(time));
  }

 private:
  int cut_size = 4;
};

ALICE_ADD_COMMAND(elm, "Mapping")
}  // namespace alice

#endif