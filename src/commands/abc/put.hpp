/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */
/**
 * @file put.hpp
 *
 * @brief converts the current GIA network into Ntk
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_PUT_HPP
#define ABC_PUT_HPP

#include "base/abc/abc.h"
// #include "base/abci/abcDar.c"
// #include "aig/gia/gia.h"
#include "aig/gia/giaAig.h"

using namespace std;
using namespace mockturtle;
using namespace pabc;

namespace alice {

class put_command : public command {
 public:
  explicit put_command(const environment::ptr &env)
      : command(env, "converts the current network into AIG") {}

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;
    begin = clock();

    Aig_Man_t *pMan;
    Abc_Ntk_t *pNtk;
    if (store<pabc::Abc_Ntk_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC AIG network\n";
    else
      pNtk = store<pabc::Abc_Ntk_t *>().current();

    if (store<pabc::Gia_Man_t *>().size() == 0u)
      std::cerr << "Error: Empty ABC GIA network\n";
    else {
      Gia_Man_t *pGia = store<pabc::Gia_Man_t *>().current();

      int fStatusClear = 1;
      int fUseBuffs = 0;
      int fFindEnables = 0;
      int fVerbose = 0;
      if (fFindEnables)
        pNtk = Abc_NtkFromMappedGia(pGia, 1, fUseBuffs);
      else if (Gia_ManHasCellMapping(pGia))
        pNtk = Abc_NtkFromCellMappedGia(pGia, fUseBuffs);
      else if (Gia_ManHasMapping(pGia) || pGia->pMuxes)
        pNtk = Abc_NtkFromMappedGia(pGia, 0, fUseBuffs);
      else if (Gia_ManHasDangling(pGia) == 0) {
        pMan = Gia_ManToAig(pGia, 0);
        pNtk = Abc_NtkFromAigPhase(pMan);
        pNtk->pName = Extra_UtilStrsav(pMan->pName);
        Aig_ManStop(pMan);
      } else {
        Abc_Ntk_t *pNtkNoCh;
        Abc_Print(-1, "Transforming AIG with %d choice nodes.\n",
                  Gia_ManEquivCountClasses(pGia));
        // create network without choices
        pMan = Gia_ManToAig(pGia, 0);
        pNtkNoCh = Abc_NtkFromAigPhase(pMan);
        pNtkNoCh->pName = Extra_UtilStrsav(pMan->pName);
        Aig_ManStop(pMan);
        // derive network with choices
        pMan = Gia_ManToAig(pGia, 1);
        pNtk = Abc_NtkFromDarChoices(pNtkNoCh, pMan);
        Abc_NtkDelete(pNtkNoCh);
        Aig_ManStop(pMan);
      }
      // transfer PI names to pNtk
      // if ( pGia->vNamesIn )
      // {
      //     Abc_Obj_t * pObj;
      //     int i;
      //     Abc_NtkForEachCi( pNtk, pObj, i ) {
      //         if (i < Vec_PtrSize(pGia->vNamesIn)) {
      //             Nm_ManDeleteIdName(pNtk->pManName, pObj->Id);
      //             Abc_ObjAssignName( pObj, (char
      //             *)Vec_PtrEntry(pGia->vNamesIn, i), NULL );
      //         }
      //     }
      // }
      // transfer PO names to pNtk
      // if ( pGia->vNamesOut )
      // {
      //     char pSuffix[100];
      //     Abc_Obj_t * pObj;
      //     int i, nDigits = Abc_Base10Log( Abc_NtkLatchNum(pNtk) );
      //     Abc_NtkForEachCo( pNtk, pObj, i ) {
      //         if (i < Vec_PtrSize(pGia->vNamesOut)) {
      //             Nm_ManDeleteIdName(pNtk->pManName, pObj->Id);
      //             if ( Abc_ObjIsPo(pObj) )
      //                 Abc_ObjAssignName( pObj, (char
      //                 *)Vec_PtrEntry(pGia->vNamesOut, i), NULL );
      //             else
      //             {
      //                 assert( i >= Abc_NtkPoNum(pNtk) );
      //                 sprintf( pSuffix, "_li%0*d", nDigits,
      //                 i-Abc_NtkPoNum(pNtk) ); Abc_ObjAssignName( pObj, (char
      //                 *)Vec_PtrEntry(pGia->vNamesOut, i), pSuffix );
      //             }
      //         }
      //     }
      // }
      // if ( pGia->vNamesNode )
      //     Abc_Print( 0, "Internal nodes names are not transferred.\n" );

      // // decouple CI/CO with the same name
      // if ( pGia->vNamesIn || pGia->vNamesOut )
      //     Abc_NtkRedirectCiCo( pNtk );

      // // transfer timing information
      // if ( pGia->vInArrs || pGia->vOutReqs )
      // {
      //     Abc_Obj_t * pObj; int i;
      //     Abc_NtkTimeInitialize( pNtk, NULL );
      //     Abc_NtkTimeSetDefaultArrival( pNtk, pGia->DefInArrs,
      //     pGia->DefInArrs ); Abc_NtkTimeSetDefaultRequired( pNtk,
      //     pGia->DefOutReqs, pGia->DefOutReqs ); if ( pGia->vInArrs )
      //         Abc_NtkForEachCi( pNtk, pObj, i )
      //             Abc_NtkTimeSetArrival( pNtk, Abc_ObjId(pObj),
      //             Vec_FltEntry(pGia->vInArrs, i), Vec_FltEntry(pGia->vInArrs,
      //             i) );
      //     if ( pGia->vOutReqs )
      //         Abc_NtkForEachCo( pNtk, pObj, i )
      //             Abc_NtkTimeSetRequired( pNtk, Abc_ObjId(pObj),
      //             Vec_FltEntry(pGia->vOutReqs, i),
      //             Vec_FltEntry(pGia->vOutReqs, i) );
      // }

      // replace the current network
      // Abc_FrameReplaceCurrentNetwork( pAbc, pNtk );
      // if ( fStatusClear )
      //     Abc_FrameClearVerifStatus( pAbc );

      store<pabc::Abc_Ntk_t *>().extend();
      store<pabc::Abc_Ntk_t *>().current() = pNtk;

      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

      cout.setf(ios::fixed);
      cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
    }
  }

 private:
};

ALICE_ADD_COMMAND(put, "ABC")

}  // namespace alice

#endif