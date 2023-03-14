/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file resub.hpp
 *
 * @brief performs technology-independent restructuring of the AIG
 *
 * @author Homyoung
 * @since  2023/02/28
 */

#ifndef ABC_RESUB_HPP
#define ABC_RESUB_HPP

#include "base/abci/abcRestruct.c"

using namespace std;
using namespace mockturtle;

namespace alice {

class aresub_command : public command {
 public:
  explicit aresub_command(const environment::ptr &env)
      : command(env,
                "performs technology-independent restructuring of the AIG") {
    add_flag("--verbose, -v", "print the information");
  }

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
      int nCutsMax = 5, fUpdateLevel = 0, fUseZeros = 0, fVerbose = 0;

      if (is_set("verbose")) fVerbose ^= 1;

      Abc_NtkRestructure(pNtk, nCutsMax, fUpdateLevel, fUseZeros, fVerbose);
      store<pabc::Abc_Ntk_t *>().extend();
      store<pabc::Abc_Ntk_t *>().current() = pNtk;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(aresub, "ABC")

}  // namespace alice

#endif