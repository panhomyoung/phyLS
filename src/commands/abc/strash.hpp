/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file strash.hpp
 *
 * @brief transforms combinational logic into an AIG
 *
 * @author Homyoung
 * @since  2023/03/01
 */

#ifndef STRASH_HPP
#define STRASH_HPP

#include "base/abc/abc.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class strash_command : public command {
 public:
  explicit strash_command(const environment::ptr &env)
      : command(env, "transforms combinational logic into an AIG") {}

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC AIG network\n";
    else {
      pabc::Abc_Ntk_t *pNtk = store<pabc::Abc_Ntk_t *>().current();

      // set defaults
      int fAllNodes = 0;
      int fRecord = 1;
      int fCleanup = 0;

      pabc::Abc_Ntk_t *pNtkRes;

      pNtkRes = Abc_NtkStrash(pNtk, fAllNodes, fCleanup, fRecord);
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

ALICE_ADD_COMMAND(strash, "ABC")

}  // namespace alice

#endif