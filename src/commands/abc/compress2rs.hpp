/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file compress2rs.hpp
 *
 * @brief combination optimization script
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef COMPRESS2RS_HPP
#define COMPRESS2RS_HPP

// #include "base/abci/abc.c"
#include "base/abc/abc.h"
// #include "base/abci/abcDar.c"
#include "base/abci/abcResub.c"

using namespace std;
using namespace mockturtle;
using namespace pabc;

namespace alice {

class compress2rs_command : public command {
 public:
  explicit compress2rs_command(const environment::ptr &env)
      : command(env, "usage: synthesis scripts.") {
    add_flag("--verbose, -v", "print the information");
  }

 public:
  void Dar_ManDefaultResParams(Dar_ResPar_t *pPars) {
    memset(pPars, 0, sizeof(Dar_ResPar_t));
    pPars->nCutsMax = 8;  // 8
    pPars->nNodesMax = 1;
    pPars->nLevelsOdc = 0;
    pPars->fUpdateLevel = 1;
    pPars->fUseZeros = 0;
    pPars->fVerbose = 0;
    pPars->fVeryVerbose = 0;
  }

  Aig_Man_t *Dar_ManCompress2rs(Abc_Ntk_t *pNtk, Aig_Man_t *pAig, int fBalance,
                                int fUpdateLevel, int fFanout, int fPower,
                                int fVerbose, int nMinSaved) {
    //    alias compress2rs "b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs
    //    -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10
    //    -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l"
    Aig_Man_t *pTemp;
    Abc_Ntk_t *pNtkAig;
    Dar_RwrPar_t ParsRwr, *pParsRwr = &ParsRwr;
    Dar_RefPar_t ParsRef, *pParsRef = &ParsRef;
    Dar_ResPar_t ParsRes, *pParsRes = &ParsRes;

    Dar_ManDefaultRwrParams(pParsRwr);
    Dar_ManDefaultRefParams(pParsRef);
    Dar_ManDefaultResParams(pParsRes);

    pParsRwr->fUpdateLevel = fUpdateLevel;
    pParsRef->fUpdateLevel = fUpdateLevel;

    pParsRes->fUpdateLevel = fUpdateLevel;
    // pParsRes->nCutsMax = nCutsMax;
    // pParsRes->nNodesMax = nNodesMax;

    pParsRwr->fFanout = fFanout;
    pParsRwr->fPower = fPower;
    pParsRwr->fVerbose = 0;  // fVerbose;
    pParsRef->fVerbose = 0;  // fVerbose;
    pParsRes->fVerbose = 0;  // fVerbose;

    pAig = Aig_ManDupDfs(pAig);
    if (fVerbose) printf("Starting:  "), Aig_ManPrintStats(pAig);
    // b -l;
    fUpdateLevel = 0;
    pAig = Dar_ManBalance(pTemp = pAig, fUpdateLevel);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Balance:   "), Aig_ManPrintStats(pAig);
    // rs -K 6 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 6;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 1;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rw -l;
    pParsRwr->fUpdateLevel = 0;
    Dar_ManRewrite(pAig, pParsRwr);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Rewrite:   "), Aig_ManPrintStats(pAig);
    // rs -K 6 -N 2 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 6;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 2;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rf -l;
    pParsRef->fUpdateLevel = 0;
    Dar_ManRefactor(pAig, pParsRef);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Refactor:  "), Aig_ManPrintStats(pAig);
    // rs -K 8 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 8;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 1;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // b -l;
    fUpdateLevel = 0;
    pAig = Dar_ManBalance(pTemp = pAig, fUpdateLevel);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Balance:   "), Aig_ManPrintStats(pAig);
    // rs -K 8 -N 2 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 8;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 2;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rw -l;
    pParsRwr->fUpdateLevel = 0;
    Dar_ManRewrite(pAig, pParsRwr);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Rewrite:   "), Aig_ManPrintStats(pAig);
    // rs -K 10 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 10;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 1;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rwz -l;
    pParsRwr->fUpdateLevel = 0;
    pParsRwr->fUseZeros = 1;
    Dar_ManRewrite(pAig, pParsRwr);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Rewrite_zerocost:   "), Aig_ManPrintStats(pAig);
    // rs -K 10 -N 2 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 10;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 2;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // b -l;
    fUpdateLevel = 0;
    pAig = Dar_ManBalance(pTemp = pAig, fUpdateLevel);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Balance:   "), Aig_ManPrintStats(pAig);
    // rs -K 12 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 12;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 1;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rfz -l;
    pParsRef->fUpdateLevel = 0;
    pParsRef->fUseZeros = 1;
    Dar_ManRefactor(pAig, pParsRef);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Refactor_zerocost:  "), Aig_ManPrintStats(pAig);
    // rs -K 12 -N 2 -l;
    pNtkAig = Abc_NtkFromDar(pNtk, pAig);
    pParsRes->nCutsMax = 12;
    pParsRes->fUpdateLevel = 0;
    pParsRes->nNodesMax = 2;
    Abc_NtkResubstitute(pNtkAig, pParsRes->nCutsMax, pParsRes->nNodesMax,
                        nMinSaved, pParsRes->nLevelsOdc, fUpdateLevel, fVerbose,
                        pParsRes->fVeryVerbose);
    pAig = Abc_NtkToDar(pNtkAig, 0, 0);
    if (fVerbose) printf("Resub:   "), Aig_ManPrintStats(pAig);
    // rwz -l;
    pParsRwr->fUpdateLevel = 0;
    pParsRwr->fUseZeros = 1;
    Dar_ManRewrite(pAig, pParsRwr);
    pAig = Aig_ManDupDfs(pTemp = pAig);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Rewrite_zerocost:   "), Aig_ManPrintStats(pAig);
    // b -l;
    fUpdateLevel = 0;
    pAig = Dar_ManBalance(pTemp = pAig, fUpdateLevel);
    Aig_ManStop(pTemp);
    if (fVerbose) printf("Balance:   "), Aig_ManPrintStats(pAig);
    return pAig;
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty AIG network.\n";
    else {
      Abc_Ntk_t *pNtk, *pNtkAig;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      int fVerbose;
      int fUpdateLevel;
      int fFanout;
      int fPower;
      int c;
      int fBalance;
      int nMinSaved;
      // set defaults
      fVerbose = 0;
      fUpdateLevel = 0;
      fFanout = 1;
      fPower = 0;
      fBalance = 0;
      nMinSaved = 1;
      Extra_UtilGetoptReset();
      if (is_set("verbose")) {
        fVerbose = 1;
      }
      if (pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return;
      }
      if (!Abc_NtkIsStrash(pNtk)) {
        Abc_Print(-1, "This command works only for strashed networks.\n");
        return;
      }
      Aig_Man_t *pMan, *pTemp;
      abctime clk;
      assert(Abc_NtkIsStrash(pNtk));
      pMan = Abc_NtkToDar(pNtk, 0, 0);
      if (pMan == NULL) return;
      clk = Abc_Clock();
      pMan = Dar_ManCompress2rs(pNtk, pTemp = pMan, fBalance, fUpdateLevel,
                                fFanout, fPower, fVerbose, nMinSaved);
      Aig_ManStop(pTemp);
      pNtkAig = Abc_NtkFromDar(pNtk, pMan);
      Aig_ManStop(pMan);
      if (pNtkAig == NULL) {
        Abc_Print(-1, "Command has failed.\n");
        return;
      }

      store<pabc::Abc_Ntk_t *>().extend();
      store<pabc::Abc_Ntk_t *>().current() = pNtkAig;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(compress2rs, "ABC")

}  // namespace alice

#endif