/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2024 */
/**
 * @file stime.hpp
 *
 * @brief performs STA using Liberty library
 *
 * @author Homyoung
 * @since  2024/10/14
 */

#ifndef STIME_HPP
#define STIME_HPP

#include "../core/read_placement_file.hpp"
#include "base/abc/abc.h"

using namespace std;
using namespace mockturtle;
using namespace pabc;

namespace alice {

class stime_command : public command {
 public:
  explicit stime_command(const environment::ptr &env)
      : command(env, "performs STA using Liberty library") {
    add_option("--lib, -l", lib_file, "Liberty library");
    add_option("--netlist, -n", netlist_file, "Mapped netlist");
    add_option("--def, -d", def_file, "Netlist with placed information");
    add_flag("--wire, -w", "using wire-loads");
    add_flag("--path, -p", "timing information for critical path");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    Abc_Ntk_t *pNtk;
    pabc::Abc_Frame_t *pAbc = pabc::Abc_FrameGetGlobalFrame();
    SC_DontUse dont_use = {0};
    SC_Lib *pLib =
        Scl_ReadLibraryFile(pAbc, (char *)(lib_file.c_str()), 1, 0, dont_use);
    ABC_FREE(dont_use.dont_use_list);
    Abc_SclLoad(pLib, (SC_Lib **)&pAbc->pLibScl);
    if (pAbc->pLibScl) {
      Abc_SclInstallGenlib(pAbc->pLibScl, 0, 0, 0);
      Mio_LibraryTransferCellIds();
    }

    if (Abc_FrameReadLibGen() == NULL) {
      Abc_Print(-1,
                "Cannot read mapped design when the library is not given.\n");
      return;
    }
    for (auto &x : netlist_file) {
      if (x == '>' || x == '\\') x = '/';
    }
    pNtk = Io_Read((char *)(netlist_file.c_str()), IO_FILE_VERILOG, 1, 0);

    // set defaults
    int fUseWireLoads = 0;
    int fPrintPath = 0;
    if (is_set("wire")) fUseWireLoads ^= 1;
    if (is_set("path")) fPrintPath ^= 1;

    if (pNtk == NULL) {
      Abc_Print(-1, "There is no current network.\n");
      return;
    }
    if (!Abc_NtkHasMapping(pNtk)) {
      Abc_Print(-1, "The current network is not mapped.\n");
      return;
    }
    if (!Abc_SclCheckNtk(pNtk, 0)) {
      Abc_Print(-1,
                "The current network is not in a topo order (run \"topo\").\n");
      return;
    }

    Vec_Int_t **VecNP;  // vector<vector<int>>
    VecNP = ABC_ALLOC(Vec_Int_t *, Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) +
                                       Abc_NtkPoNum(pNtk));
    for (int i = 0;
         i < Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk);
         i++)
      VecNP[i] = Vec_IntAlloc(2);
    phyLS::read_def_file_abc(def_file, VecNP);

    for (int j = 0;
         j < Abc_NtkNodeNum(pNtk) + Abc_NtkPiNum(pNtk) + Abc_NtkPoNum(pNtk);
         j++) {
      for (int i = 0; i < Vec_IntSize(VecNP[j]); i++)
        std::cout << Vec_IntEntry(VecNP[j], i) << " ";
      std::cout << std::endl;
    }

    pNtk->vPlace = VecNP;

    Abc_SclTimePerform(pLib, pNtk, 0, fUseWireLoads, 0, fPrintPath, 0);

    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  string lib_file, netlist_file, def_file;
};

ALICE_ADD_COMMAND(stime, "ABC")

}  // namespace alice

#endif