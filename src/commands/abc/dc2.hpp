/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file dc2.hpp
 *
 * @brief combination optimization script
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef DC2_HPP
#define DC2_HPP

// #include "base/abci/abc.c"
#include "base/abc/abc.h"
// #include "base/abci/abcDar.c"

using namespace std;
using namespace mockturtle;
using namespace pabc;
namespace alice {

class dc2_command : public command {
 public:
  explicit dc2_command(const environment::ptr &env)
      : command(env, "usage: dc2 [-blfpvh]") {
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
      Abc_Ntk_t *pNtk, *pNtkRes;
      int fBalance, fVerbose, fUpdateLevel, fFanout, fPower, c;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      // set defaults
      fBalance = 0;
      fVerbose = 0;
      fUpdateLevel = 0;
      fFanout = 1;
      fPower = 0;
      Extra_UtilGetoptReset();
      if (pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return;
      }
      if (!Abc_NtkIsStrash(pNtk)) {
        Abc_Print(-1, "This command works only for strashed networks.\n");
        return;
      }
      pNtkRes =
          Abc_NtkDC2(pNtk, fBalance, fUpdateLevel, fFanout, fPower, fVerbose);
      if (pNtkRes == NULL) {
        Abc_Print(-1, "Command has failed.\n");
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

ALICE_ADD_COMMAND(dc2, "ABC")

}  // namespace alice

#endif