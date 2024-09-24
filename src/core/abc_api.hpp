/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file abc_api.hpp
 *
 * @brief ABC interface
 *
 * @author Jiaxiang Pan, Jun zhu
 * @since  2024/09/20
 */

#ifndef ABC_API_HPP
#define ABC_API_HPP

#include <base/main/main.h>
#include <aig/aig/aig.h>
#include <aig/gia/gia.h>
#include <misc/util/abc_global.h>
#include <sat/cnf/cnf.h>

namespace pabc {
//void Abc_UtilsPrintHello( Abc_Frame_t * pAbc );
void Abc_UtilsSource( Abc_Frame_t * pAbc );
char * Abc_UtilsGetUsersInput( Abc_Frame_t * pAbc );
void Mf_ManDumpCnf( Gia_Man_t * p, char * pFileName, int nLutSize, int fCnfObjIds, int fAddOrCla, int fVerbose );
void * Mf_ManGenerateCnf( Gia_Man_t * pGia, int nLutSize, int fCnfObjIds, int fAddOrCla, int fMapping, int fVerbose );
Aig_Man_t * Gia_ManToAig( Gia_Man_t * p, int fChoices );
Gia_Man_t * Gia_ManFromAig( Aig_Man_t * p );
Abc_Ntk_t * Abc_NtkFromAigPhase( Aig_Man_t * pMan );
Aig_Man_t * Abc_NtkToDar( Abc_Ntk_t * pNtk, int fExors, int fRegisters );
}

#endif
