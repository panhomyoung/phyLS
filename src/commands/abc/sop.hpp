/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file sop.hpp
 *
 * @brief converts node functions to SOP
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef SOP_HPP
#define SOP_HPP

// #include "base/abci/abc.c"
#include "base/abc/abc.h"
#include "base/abc/abcFunc.c"

using namespace std;
using namespace mockturtle;
using namespace pabc;
namespace alice {

class sop_command : public command {
 public:
  explicit sop_command(const environment::ptr &env)
      : command(env, "converts node functions to SOP") {
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
      Abc_Ntk_t *pNtk;
      pNtk = store<pabc::Abc_Ntk_t *>().current();
      int c, fCubeSort = 1, fMode = -1, nCubeLimit = 1000000;
      // set defaults
      Extra_UtilGetoptReset();

      if (pNtk == NULL) {
        Abc_Print(-1, "Empty network.\n");
        return;
      }
      if (!Abc_NtkIsLogic(pNtk)) {
        Abc_Print(-1,
                  "Converting to SOP is possible only for logic networks.\n");
        return;
      }
      if (!fCubeSort && Abc_NtkHasBdd(pNtk) &&
          !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY, 0)) {
        Abc_Print(-1, "Converting to SOP has failed.\n");
        return;
      }
      if (!Abc_NtkToSop(pNtk, fMode, nCubeLimit)) {
        Abc_Print(-1, "Converting to SOP has failed.\n");
        return;
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

ALICE_ADD_COMMAND(sop, "ABC")

}  // namespace alice

#endif