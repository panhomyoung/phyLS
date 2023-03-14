/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file read.hpp
 *
 * @brief  replaces the current network by the network read from file
 *
 * @author Homyoung
 * @since  2023/03/14
 */

#ifndef READ_HPP
#define READ_HPP

#include "base/abc/abc.h"

using namespace std;
using namespace mockturtle;

namespace alice {

class read_command : public command {
 public:
  explicit read_command(const environment::ptr &env)
      : command(env,
                "replaces the current network by the network read from file by "
                "ABC parser") {
    add_option("filename,-f", file_name, "name of input file");
    add_flag("--check, -c", "Disable network check after reading");
    add_flag("--buffer, -b", "Reading barrier buffers");
  }

 protected:
  void execute() override {
    int fCheck = 1, fBarBufs = 0;
    if (is_set("check")) fCheck ^= 1;
    if (is_set("buffer")) fBarBufs ^= 1;

    // fix the wrong symbol
    for (auto &x : file_name) {
      if (x == '>' || x == '\\') x = '/';
    }

    assert(strlen((char *)(file_name.c_str())) < 900);

    pabc::Abc_Ntk_t *pNtk = pabc::Io_Read(
        (char *)(file_name.c_str()),
        pabc::Io_ReadFileType((char *)(file_name.c_str())), fCheck, fBarBufs);

    if (pNtk == NULL) std::cerr << "Error: Empty input format.\n";
    store<pabc::Abc_Ntk_t *>().extend();
    store<pabc::Abc_Ntk_t *>().current() = pNtk;
  }

 private:
  std::string file_name;
};

ALICE_ADD_COMMAND(read, "I/O")

}  // namespace alice

#endif