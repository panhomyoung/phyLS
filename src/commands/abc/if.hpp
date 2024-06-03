/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file if.hpp
 *
 * @brief performs FPGA technology mapping of the network
 *
 * @author Homyoung
 * @since  2023/08/03
 */

#ifndef ABC_IF_HPP
#define ABC_IF_HPP

#include "base/abc/abc.h"
#include "base/abci/abcIf.c"
#include "base/main/main.h"
#include "map/if/if.h"

using namespace mockturtle;
using namespace std;
using namespace pabc;

namespace alice {

class if_command : public command {
 public:
  explicit if_command(const environment::ptr &env)
      : command(env, "performs FPGA mapping of the AIG") {
    add_option("-k,--klut", LutSize, "the number of LUT inputs (2 < k < 8), Default=4");
    add_option("-c,--cut", CutSize,
               "the max number of priority cuts (0 < c < 2^12), Default=8");
    add_flag("--minimization, -m",
             "enables cut minimization by removing vacuous variables");
    add_flag("--delay, -y", "delay optimization with recorded library");
    add_option("-f,--file", filename, "recorded library");
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
      Abc_Ntk_t *pNtk = store<pabc::Abc_Ntk_t *>().current();
      // set defaults
      Abc_Ntk_t *pNtkRes;
      If_Par_t Pars, *pPars = &Pars;
      If_ManSetDefaultPars(pPars);
      pPars->pLutLib = (If_LibLut_t *)Abc_FrameReadLibLut();

      if (is_set("klut")) pPars->nLutSize = LutSize;
      if (is_set("cut")) pPars->nCutsMax = CutSize;
      if (is_set("minimization")) pPars->fCutMin ^= 1;
      if (is_set("delay")) {
        pPars->fUserRecLib ^= 1;
        pPars->fTruth = 1;
        pPars->fCutMin = 1;
        pPars->fExpRed = 0;
        pPars->fUsePerm = 1;
        pPars->pLutLib = NULL;
        int nVars = 6;
        int nCuts = 32;
        int fFuncOnly = 0;
        int fVerbose = 0;
        char *FileName, *pTemp;
        FILE *pFile;
        Gia_Man_t *pGia = NULL;
        FileName = filename.data();
        for (pTemp = FileName; *pTemp; pTemp++)
          if (*pTemp == '>') *pTemp = '\\';
        if ((pFile = fopen(FileName, "r")) == NULL) {
          printf("Cannot open input file \"%s\". ", FileName);
          if ((FileName = Extra_FileGetSimilarName(FileName, ".aig", NULL, NULL,
                                                   NULL, NULL)))
            printf("Did you mean \"%s\"?", FileName);
          printf("\n");
        }
        fclose(pFile);
        pGia = Gia_AigerRead(FileName, 0, 1, 0);
        if (pGia == NULL) {
          printf("Reading AIGER has failed.\n");
        }
        Abc_NtkRecStart3(pGia, nVars, nCuts, fFuncOnly, fVerbose);
      }

      if (!Abc_NtkIsStrash(pNtk)) {
        // strash and balance the network
        pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
        if (pNtk == NULL) {
          printf("Strashing before FPGA mapping has failed.\n");
          return;
        }
        pNtk = Abc_NtkBalance(pNtkRes = pNtk, 0, 0, 1);
        Abc_NtkDelete(pNtkRes);
        if (pNtk == NULL) {
          printf("Balancing before FPGA mapping has failed.\n");
          return;
        }
        if (!Abc_FrameReadFlag("silentmode"))
          printf(
              "The network was strashed and balanced before FPGA mapping.\n");
        // get the new network
        pNtkRes = Abc_NtkIf(pNtk, pPars);
        if (pNtkRes == NULL) {
          Abc_NtkDelete(pNtk);
          printf("FPGA mapping has failed.\n");
          return;
        }
        Abc_NtkDelete(pNtk);
      } else {
        // get the new network
        pNtkRes = Abc_NtkIf(pNtk, pPars);
        if (pNtkRes == NULL) {
          printf("FPGA mapping has failed.\n");
          return;
        }
      }
      store<pabc::Abc_Ntk_t *>().extend();
      store<pabc::Abc_Ntk_t *>().current() = pNtkRes;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  uint32_t LutSize = 4u;
  uint32_t CutSize = 8u;
  string filename;
};

ALICE_ADD_COMMAND(if, "ABC")

}  // namespace alice

#endif