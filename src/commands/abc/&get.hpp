/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file &get.hpp
 *
 * @brief converts the current network into GIA
 *
 * @author Homyoung
 * @since  2023/03/14
 */

#ifndef AGET_HPP
#define AGET_HPP

#include "aig/gia/giaAig.h"
#include "base/abc/abc.h"
#include "base/abci/abcDar.c"

using namespace std;
using namespace mockturtle;

namespace alice {

class Aget_command : public command {
 public:
  explicit Aget_command(const environment::ptr &env)
      : command(env, "converts the current network into GIA") {}

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC AIG network\n";
    else {
      pabc::Abc_Ntk_t *pNtk = store<pabc::Abc_Ntk_t *>().current();

      pabc::Aig_Man_t *pAig;
      pabc::Gia_Man_t *pGia;

      if (pabc::Abc_NtkGetChoiceNum(pNtk))
        pAig = Abc_NtkToDarChoices(pNtk);
      else
        pAig = Abc_NtkToDar(pNtk, 0, 1);

      pGia = Gia_ManFromAig(pAig);
      pabc::Aig_ManStop(pAig);
      store<pabc::Gia_Man_t *>().extend();
      store<pabc::Gia_Man_t *>().current() = pGia;
    }

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
};

ALICE_ADD_COMMAND(Aget, "Gia")

}  // namespace alice

#endif