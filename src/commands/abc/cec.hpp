/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file cec.hpp
 *
 * @brief performs combinational equivalence checking
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef CEC2_HPP
#define CEC2_HPP

#include <vector>

#include "base/abc/abc.h"
#include "base/abci/abcVerify.c"

using namespace std;
using namespace mockturtle;

namespace alice {

class acec_command : public command {
 public:
  explicit acec_command(const environment::ptr &env)
      : command(env, "performs combinational equivalence checking") {
    add_option("file1, -f", filename_1, "the input file name");
    add_option("file2, -b", filename_2, "the input file name");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();

    pabc::Abc_Ntk_t *pNtk;
    int fDelete1 = 0, fDelete2 = 0;
    int nArgcNew = 0;
    char **pArgvNew;
    int fCheck = 1, fBarBufs = 0;

    pabc::Abc_Ntk_t *pNtk1 = pabc::Io_Read(
        (char *)(filename_1.c_str()),
        pabc::Io_ReadFileType((char *)(filename_1.c_str())), fCheck, fBarBufs);
    end = clock();
    pabc::Abc_Ntk_t *pNtk2 = pabc::Io_Read(
        (char *)(filename_2.c_str()),
        pabc::Io_ReadFileType((char *)(filename_2.c_str())), fCheck, fBarBufs);

    // set defaults
    int fSat = 0;
    int fVerbose = 0;
    int nSeconds = 20;
    int nPartSize = 0;
    int nConfLimit = 10000;
    int nInsLimit = 0;
    int fPartition = 0;
    int fIgnoreNames = 0;

    pabc::Abc_NtkCecFraig(pNtk1, pNtk2, nSeconds, fVerbose);

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  string filename_1;
  string filename_2;
};

ALICE_ADD_COMMAND(acec, "ABC")

}  // namespace alice

#endif