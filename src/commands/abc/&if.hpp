/* LogicMap: powerful  technology-mapping tool in logic synthesis
 * Copyright (C) 2023 */

/**
 * @file &if.hpp
 *
 * @brief performs FPGA mapping of the GIA
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_GIA_IF_HPP
#define ABC_GIA_IF_HPP

#include "aig/gia/gia.h"
#include "base/abc/abc.h"

using namespace mockturtle;
using namespace std;
using namespace pabc;

namespace alice {

class Aif_command : public command {
 public:
  explicit Aif_command(const environment::ptr &env)
      : command(env, "performs FPGA mapping of the AIG") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;
    begin = clock();
    Gia_Man_t *pGia, *pNew;
    If_Par_t Pars, *pPars = &Pars;
    Gia_ManSetIfParsDefault(pPars);
    if (store<pabc::Gia_Man_t *>().size() == 0u)
      std::cerr << "Error: Empty GIA network.\n";
    else {
      pGia = store<pabc::Gia_Man_t *>().current();
      // set defaults
      pPars->pLutLib = (If_LibLut_t *)Abc_FrameReadLibLut();
      if (Gia_ManHasMapping(pGia)) {
        printf("Current AIG has mapping. Run \"&st\".\n");
        return;
      }
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

      // get the new network
      pNew = Gia_ManPerformMapping(pGia, pPars);
      if (pNew == NULL) {
        printf("Mapping of GIA has failed..\n");
        return;
      }
      store<pabc::Gia_Man_t *>().extend();
      store<pabc::Gia_Man_t *>().current() = pNew;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(Aif, "Gia")

}  // namespace alice

#endif