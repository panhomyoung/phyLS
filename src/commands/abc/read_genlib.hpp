/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file read_genlib.hpp
 *
 * @brief read the library from a genlib file
 *
 * @author Homyoung
 * @since  2023/11/16
 */

#ifndef ABC_READ_GENLIB_HPP
#define ABC_READ_GENLIB_HPP

#include "base/abc/abc.h"
#include "base/io/ioAbc.h"
#include "base/main/abcapis.h"
#include "base/main/main.h"
#include "map/amap/amap.h"
#include "map/mapper/mapper.h"
#include "map/mio/mio.h"
#include "misc/extra/extra.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class read_agenlib_command : public command {
 public:
  explicit read_agenlib_command(const environment::ptr &env)
      : command(env, "read the library from a genlib file") {
    add_option("filename,-f", file_name, "name of input file");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;
    begin = clock();

    pabc::Abc_Frame_t *pAbc;
    pAbc = pabc::Abc_FrameGetGlobalFrame();

    // set defaults
    FILE *pFile;
    // FILE * pOut, * pErr;
    pabc::Mio_Library_t *pLib;
    pabc::Amap_Lib_t *pLib2;
    char *pFileName;
    char *pExcludeFile = NULL;
    int fVerbose = 1;

    // get the input file name
    pFileName = (char *)malloc(file_name.length() + 1);
    if (pFileName != NULL) {
      strcpy(pFileName, file_name.c_str());
    } else {
      printf("Memory allocation failed\n");
    }

    // if ( (pFile = pabc::Io_FileOpen( pFileName, "open_path", "r", 0 )) ==
    // NULL )
    // {
    //     printf("Cannot open input file \"%s\". ", pFileName );
    //     if ( (pFileName = pabc::Extra_FileGetSimilarName( pFileName,
    //     ".genlib", ".lib", ".scl", ".g", NULL )) )
    //         printf( "Did you mean \"%s\"?", pFileName );
    //     printf( "\n" );
    //     return;
    // }
    // fclose( pFile );

    // set the new network
    pLib = pabc::Mio_LibraryRead(pFileName, NULL, pExcludeFile, fVerbose);
    if (pLib == NULL) {
      // fprintf( pErr, "Reading genlib library has failed.\n" );
      printf("Reading genlib library has failed.\n");
      return;
    }
    if (fVerbose)
      printf("Entered genlib library with %d gates from file \"%s\".\n",
             Mio_LibraryReadGateNum(pLib), pFileName);

    // prepare libraries
    pabc::Mio_UpdateGenlib(pLib);

    // replace the current library
    pLib2 = pabc::Amap_LibReadAndPrepare(pFileName, NULL, 0, 0);
    if (pLib2 == NULL) {
      // fprintf( pErr, "Reading second genlib library has failed.\n" );
      printf("Reading second genlib library has failed.\n");
      return;
    }
    Abc_FrameSetLibGen2(pLib2);
  }

 private:
  std::string file_name;
};

ALICE_ADD_COMMAND(read_agenlib, "I/O")

}  // namespace alice

#endif