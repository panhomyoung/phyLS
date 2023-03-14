/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file &fraig.hpp
 *
 * @brief performs combinational SAT sweeping
 *
 * @author Homyoung
 * @since  2023/03/14
 */

#ifndef AFRAIG_HPP
#define AFRAIG_HPP

#include "aig/gia/gia.h"
#include "proof/cec/cec.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class Afraig_command : public command {
 public:
  explicit Afraig_command(const environment::ptr &env)
      : command(env, "performs combinational SAT sweeping") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Gia_Man_t *>().size() == 0u)
      std::cerr << "Error: Empty GIA network.\n";
    else {
      pabc::Gia_Man_t *pNtk, *pTemp;
      pNtk = store<pabc::Gia_Man_t *>().current();

      pabc::Cec_ParFra_t ParsFra, *pPars = &ParsFra;
      // set defaults
      memset(pPars, 0, sizeof(pabc::Cec_ParFra_t));
      pPars->jType = 2;           // solver type
      pPars->fSatSweeping = 1;    // conflict limit at a node
      pPars->nWords = 4;          // simulation words
      pPars->nRounds = 10;        // simulation rounds
      pPars->nItersMax = 2000;    // this is a miter
      pPars->nBTLimit = 1000000;  // use logic cones
      pPars->nBTLimitPo = 0;      // use logic outputs
      pPars->nSatVarMax =
          1000;  // the max number of SAT variables before recycling SAT solver
      pPars->nCallsRecycle =
          500;                 // calls to perform before recycling SAT solver
      pPars->nGenIters = 100;  // pattern generation iterations

      pTemp = Cec_ManSatSweeping(pNtk, pPars, 0);

      store<pabc::Gia_Man_t *>().extend();
      store<pabc::Gia_Man_t *>().current() = pTemp;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(Afraig, "Gia")

}  // namespace alice

#endif