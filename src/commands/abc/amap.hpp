/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file amap.hpp
 *
 * @brief performs technology-independent mapping of the AIG
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_AMAP_HPP
#define ABC_AMAP_HPP

#include "map/amap/amap.h"
#include "base/abc/abc.h"
#include "base/abci/abcSweep.c"

using namespace mockturtle;
using namespace std;
using namespace pabc;

namespace alice {

class amap_command : public command {
 public:
  explicit amap_command(const environment::ptr &env)
      : command(env, "performs standard cell mapping of the current network") {
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
      Amap_Par_t Pars, *pPars = &Pars;
      Abc_Ntk_t *pNtkRes;
      int fSweep = 0;
      Amap_ManSetDefaultParams(pPars);

      if (!Abc_NtkIsStrash(pNtk)) {
        pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
        if (pNtk == NULL) {
          printf("Strashing before mapping has failed.\n");
          return;
        }
        pNtk = Abc_NtkBalance(pNtkRes = pNtk, 0, 0, 1);
        Abc_NtkDelete(pNtkRes);
        if (pNtk == NULL) {
          printf("Balancing before mapping has failed.\n");
          return;
        }
        printf("The network was strashed and balanced before mapping.\n");
        // get the new network
        pNtkRes = Abc_NtkDarAmap(pNtk, pPars);
        if (pNtkRes == NULL) {
          Abc_NtkDelete(pNtk);
          printf("Mapping has failed.\n");
          return;
        }
        Abc_NtkDelete(pNtk);
      } else {
        // get the new network
        pNtkRes = Abc_NtkDarAmap(pNtk, pPars);
        if (pNtkRes == NULL) {
          printf("Mapping has failed.\n");
          return;
        }
      }

      if (fSweep) {
        Abc_NtkFraigSweep(pNtkRes, 0, 0, 0, 0);
        if (Abc_NtkHasMapping(pNtkRes)) {
          pNtkRes = Abc_NtkDupDfs(pNtk = pNtkRes);
          Abc_NtkDelete(pNtk);
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

ALICE_ADD_COMMAND(amap, "ABC")

}  // namespace alice

#endif