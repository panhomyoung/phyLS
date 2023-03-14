/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file fraig.hpp
 *
 * @brief transforms a logic network into a functionally reduced AIG
 *
 * @author Homyoung
 * @since  2023/03/14
 */

#ifndef FRAIG_HPP
#define FRAIG_HPP

#include "base/abc/abc.h"
#include "proof/fraig/fraig.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class fraig_command : public command {
 public:
  explicit fraig_command(const environment::ptr &env)
      : command(env,
                "transforms a logic network into a functionally reduced AIG") {
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
      if (!pabc::Abc_NtkIsLogic(pNtk) && !pabc::Abc_NtkIsStrash(pNtk))
        std::cerr << "Can only fraig a logic network or an AIG.\n";

      pabc::Fraig_Params_t Params, *pParams = &Params;

      // set defaults
      int fAllNodes = 0, fExdc = 0;

      memset(pParams, 0, sizeof(pabc::Fraig_Params_t));
      pParams->nPatsRand =
          2048;  // the number of words of random simulation info
      pParams->nPatsDyna =
          2048;  // the number of words of dynamic simulation info
      pParams->nBTLimit = 100;  // the max number of backtracks to perform
      pParams->fFuncRed = 1;    // performs only one level hashing
      pParams->fFeedBack = 1;   // enables solver feedback
      pParams->fDist1Pats = 1;  // enables distance-1 patterns
      pParams->fDoSparse = 1;   // performs equiv tests for sparse functions
      pParams->fChoicing = 0;   // enables recording structural choices
      pParams->fTryProve = 0;   // tries to solve the final miter
      pParams->fVerbose = 0;    // the verbosiness flag
      pParams->fVerboseP = 0;   // the verbosiness flag

      if (is_set("verbose")) pParams->fVerbose ^= 1;

      if (pabc::Abc_NtkIsStrash(pNtk))
        pNtkRes = pabc::Abc_NtkFraig(pNtk, &Params, fAllNodes, fExdc);
      else {
        pNtk = pabc::Abc_NtkStrash(pNtk, fAllNodes, !fAllNodes, 0);
        pNtkRes = pabc::Abc_NtkFraig(pNtk, &Params, fAllNodes, fExdc);
        pabc::Abc_NtkDelete(pNtk);
      }

      if (pNtkRes == NULL) std::cerr << "Fraiging has failed.\n";

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

ALICE_ADD_COMMAND(fraig, "ABC")

}  // namespace alice

#endif