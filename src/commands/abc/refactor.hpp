/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file refactor.hpp
 *
 * @brief performs technology-independent refactoring of the AIG
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_REFACTOR_HPP
#define ABC_REFACTOR_HPP

#include "base/abc/abc.h"
#include "base/abci/abc.c"
#include "base/abci/abcRefactor.c"

using namespace std;
using namespace mockturtle;
using namespace pabc;

namespace alice {

class arefactor_command : public command {
 public:
  explicit arefactor_command(const environment::ptr &env)
      : command(env, "performs technology-independent refactoring of the AIG") {
    add_flag("--zero, -z", "using zero-cost replacements");
    add_flag("--level, -l", "preserving the number of levels");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty AIG network.\n";
    else {
      pabc::Abc_Ntk_t *pNtk, *pDup;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      int c, RetValue;
      int nNodeSizeMax;
      int nConeSizeMax;
      int fUpdateLevel;
      int fUseZeros;
      int fUseDcs;
      int fVerbose;
      int nMinSaved;

      // set defaults
      nNodeSizeMax = 10;
      nConeSizeMax = 16;
      fUpdateLevel = 1;
      fUseZeros = 0;
      fUseDcs = 0;
      fVerbose = 0;
      nMinSaved = 1;
      if (is_set("level")) fUpdateLevel ^= 1;
      if (is_set("zero")) fUseZeros ^= 1;
      Extra_UtilGetoptReset();
      if (pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return;
      }
      if (!Abc_NtkIsStrash(pNtk)) {
        Abc_Print(
            -1,
            "This command can only be applied to an AIG (run \"strash\").\n");
        return;
      }
      if (Abc_NtkGetChoiceNum(pNtk)) {
        Abc_Print(
            -1,
            "AIG resynthesis cannot be applied to AIGs with choice nodes.\n");
        return;
      }
      if (nNodeSizeMax > 15) {
        Abc_Print(-1, "The cone size cannot exceed 15.\n");
        return;
      }

      if (fUseDcs && nNodeSizeMax >= nConeSizeMax) {
        Abc_Print(-1,
                  "For don't-care to work, containing cone should be larger "
                  "than collapsed node.\n");
        return;
      }

      // modify the current network
      pDup = Abc_NtkDup(pNtk);
      RetValue = Abc_NtkRefactor(pNtk, nNodeSizeMax, nMinSaved, nConeSizeMax,
                                 fUpdateLevel, fUseZeros, fUseDcs, fVerbose);
      if (RetValue == -1) {
        // Abc_FrameReplaceCurrentNetwork(pAbc, pDup);
        printf(
            "An error occurred during computation. The original network is "
            "restored.\n");
      } else {
        Abc_NtkDelete(pDup);
        if (RetValue == 0) {
          Abc_Print(0, "Refactoring has failed.\n");
          return;
        }
      }
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

ALICE_ADD_COMMAND(arefactor, "ABC")

}  // namespace alice

#endif