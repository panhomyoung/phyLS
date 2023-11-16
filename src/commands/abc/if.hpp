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
      if (pPars->nLutSize == -1) {
        if (pPars->pLutLib == NULL) {
          printf("The LUT library is not given.\n");
          return;
        }
        pPars->nLutSize = pPars->pLutLib->LutMax;
      }

      if (pPars->nLutSize < 2 || pPars->nLutSize > IF_MAX_LUTSIZE) {
        printf("Incorrect LUT size (%d).\n", pPars->nLutSize);
        return;
      }

      if (pPars->nCutsMax < 1 || pPars->nCutsMax >= (1 << 12)) {
        printf("Incorrect number of cuts.\n");
        return;
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
};

ALICE_ADD_COMMAND(if, "ABC")

}  // namespace alice

#endif