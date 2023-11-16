/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file lutrw.hpp
 *
 * @brief 2-lut rewriting with STP based exact synthesis
 *
 * @author Homyoung
 * @since  2022/11/30
 */

#ifndef LUTRW_COMMAND_HPP
#define LUTRW_COMMAND_HPP

#include <time.h>

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <percy/percy.hpp>

#include "../../core/exact/lut_rewriting.hpp"
#include "../../core/misc.hpp"

using namespace std;
using namespace percy;
using kitty::dynamic_truth_table;

namespace alice {
class lutrw_command : public command {
 public:
  explicit lutrw_command(const environment::ptr& env)
      : command(env, "Performs 2-LUT rewriting") {
    add_flag("--techmap, -t", "rewriting by the lowest cost of realization");
    add_flag("--enumeration_techmap, -e",
             "rewriting by the lowest cost of enumerated realization");
    add_flag("--cec, -c,", "apply equivalence checking in rewriting");
    add_flag("--xag, -g", "enable exact synthesis for XAG, default = false");
  }

 protected:
  void execute() {
    mockturtle::klut_network klut = store<mockturtle::klut_network>().current();
    mockturtle::klut_network klut_orig, klut_opt;
    klut_orig = klut;
    phyLS::lut_rewriting_params ps;
    if (is_set("xag")) ps.xag = true;

    clock_t begin, end;
    double totalTime;
    if (is_set("techmap")) {
      begin = clock();
      klut_opt = phyLS::lut_rewriting_s(klut, ps);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
    } else if (is_set("enumeration_techmap")) {
      begin = clock();
      klut_opt = phyLS::lut_rewriting_s_enu(klut, ps);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
    } else {
      begin = clock();
      klut_opt = phyLS::lut_rewriting_c(klut, ps);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
    }

    if (is_set("cec")) {
      /* equivalence checking */
      const auto miter_klut = *miter<klut_network>(klut_orig, klut_opt);
      equivalence_checking_stats eq_st;
      const auto result = equivalence_checking(miter_klut, {}, &eq_st);
      assert(*result);
    }

    std::cout << "[lutrw] ";
    phyLS::print_stats(klut_opt);

    store<mockturtle::klut_network>().extend();
    store<mockturtle::klut_network>().current() = klut_opt;

    cout.setf(ios::fixed);
    cout << "[Total CPU time]   : " << setprecision(3) << totalTime << " s"
         << endl;
  }

 private:
};

ALICE_ADD_COMMAND(lutrw, "Synthesis")
}  // namespace alice

#endif