/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file &nf.hpp
 *
 * @brief performs technology-independent mapping of the GIA
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_NF_HPP
#define ABC_NF_HPP

#include "aig/gia/gia.h"
#include "base/abc/abc.h"
#include "aig/gia/giaNf.c"

using namespace mockturtle;
using namespace std;
using namespace pabc;

namespace alice {

class Anf_command : public command {
 public:
  explicit Anf_command(const environment::ptr &env)
      : command(env, "performs technology mapping of the GIA network") {
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    char Buffer[200];
    pabc::Jf_Par_t Pars, *pPars = &Pars;
    pabc::Gia_Man_t *pGia, *pNew;
    pabc::Nf_ManSetDefaultPars(pPars);

    if (store<pabc::Gia_Man_t *>().size() == 0u)
      std::cerr << "Error: Empty GIA network.\n";
    else {
      pGia = store<pabc::Gia_Man_t *>().current();
      if (pabc::Abc_FrameReadLibGen() == NULL) {
        pabc::Abc_Print(-1, "Current library is not available.\n");
        return;
      }

      pNew = pabc::Nf_ManPerformMapping(pGia, pPars);
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

ALICE_ADD_COMMAND(Anf, "Gia")

}  // namespace alice

#endif