/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file map.hpp
 *
 * @brief performs technology-independent mapping of the AIG
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_MAP_HPP
#define ABC_MAP_HPP

#include "base/abc/abc.h"
#include "base/abci/abcMap.c"

using namespace mockturtle;
using namespace std;

namespace alice {

class map_command : public command {
 public:
  explicit map_command(const environment::ptr &env)
      : command(env, "performs standard cell mapping of the current network") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    pabc::Abc_Ntk_t *pNtkRes;
    double DelayTarget;
    double AreaMulti;
    double DelayMulti;
    float LogFan = 0;
    float Slew = 0;  // choose based on the library
    float Gain = 250;
    int nGatesMin = 0;
    int fAreaOnly;
    int fRecovery;
    int fSweep;
    int fSwitching;
    int fSkipFanout;
    int fUseProfile;
    int fUseBuffs;
    int fVerbose;

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC AIG network\n";
    else {
      pabc::Abc_Ntk_t *pNtk = store<pabc::Abc_Ntk_t *>().current();

      // set defaults
      DelayTarget = -1;
      AreaMulti = 0;
      DelayMulti = 0;
      fAreaOnly = 0;
      fRecovery = 1;
      fSweep = 0;
      fSwitching = 0;
      fSkipFanout = 0;
      fUseProfile = 0;
      fUseBuffs = 0;
      fVerbose = 0;

      if (is_set("verbose")) fVerbose ^= 1;

      pNtkRes = Abc_NtkMap(pNtk, DelayTarget, AreaMulti, DelayMulti, LogFan,
                           Slew, Gain, nGatesMin, fRecovery, fSwitching,
                           fSkipFanout, fUseProfile, fUseBuffs, fVerbose);
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

ALICE_ADD_COMMAND(map, "ABC")

}  // namespace alice

#endif