/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file balance.hpp
 *
 * @brief transforms the current network into a well-balanced AIG
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_BALANCE_HPP
#define ABC_BALANCE_HPP

// #include "base/abci/abc.c"
#include "base/abc/abc.h"
#include "base/abci/abcBalance.c"

using namespace std;
using namespace mockturtle;
using namespace pabc;
namespace alice {

class abalance_command : public command {
 public:
  explicit abalance_command(const environment::ptr &env)
      : command(env,
                "transforms the current network into a well-balanced AIG") {
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
      Abc_Ntk_t *pNtk, *pNtkRes, *pNtkTemp;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      int c;
      int fDuplicate;
      int fSelective;
      int fUpdateLevel;
      int fExor;
      int fVerbose;
      // set defaults
      fDuplicate = 0;
      fSelective = 0;
      fUpdateLevel = 1;
      fExor = 0;
      fVerbose = 0;
      Extra_UtilGetoptReset();
      if (pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return;
      }
      // get the new network
      if (Abc_NtkIsStrash(pNtk)) {
        if (fExor)
          pNtkRes = Abc_NtkBalanceExor(pNtk, fUpdateLevel, fVerbose);
        else
          pNtkRes = Abc_NtkBalance(pNtk, fDuplicate, fSelective, fUpdateLevel);
      } else {
        pNtkTemp = Abc_NtkStrash(pNtk, 0, 0, 0);
        if (pNtkTemp == NULL) {
          Abc_Print(-1, "Strashing before balancing has failed.\n");
          return;
        }
        if (fExor)
          pNtkRes = Abc_NtkBalanceExor(pNtkTemp, fUpdateLevel, fVerbose);
        else
          pNtkRes =
              Abc_NtkBalance(pNtkTemp, fDuplicate, fSelective, fUpdateLevel);
        Abc_NtkDelete(pNtkTemp);
      }

      // check if balancing worked
      if (pNtkRes == NULL) {
        Abc_Print(-1, "Balancing has failed.\n");
        return;
      }
      // replace the current network
      // Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes);

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

ALICE_ADD_COMMAND(abalance, "ABC")

}  // namespace alice

#endif