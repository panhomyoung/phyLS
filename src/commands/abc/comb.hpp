/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file comb.hpp
 *
 * @brief converts comb network into seq, and vice versa
 *
 * @author Homyoung
 * @since  2023/03/21
 */

#ifndef COMB_HPP
#define COMB_HPP

#include "base/abc/abc.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class comb_command : public command {
 public:
  explicit comb_command(const environment::ptr &env)
      : command(env, "converts comb network into seq, and vice versa") {
    add_flag(
        "--convert, -c",
        "converting latches to PIs/POs or removing them [default = convert]");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC AIG network.\n";
    else {
      pabc::Abc_Ntk_t *pNtk, *pNtkRes;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      if (pabc::Abc_NtkIsComb(pNtk))
        std::cerr << "The network is already combinational.\n";

      int fRemoveLatches = 0;
      if (is_set("convert")) fRemoveLatches ^= 1;

      pNtkRes = pabc::Abc_NtkDup(pNtk);
      pabc::Abc_NtkMakeComb(pNtkRes, fRemoveLatches);

      store<pabc::Abc_Ntk_t *>().extend();
      store<pabc::Abc_Ntk_t *>().current() = pNtkRes;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(comb, "ABC")

}  // namespace alice

#endif