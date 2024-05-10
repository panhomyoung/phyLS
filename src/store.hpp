/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef STORE_HPP
#define STORE_HPP

#include <alice/alice.hpp>
#include <kitty/kitty.hpp>
#include <lorina/diagnostics.hpp>
#include <lorina/genlib.hpp>
#include <mockturtle/io/blif_reader.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/io/write_aiger.hpp>
#include <mockturtle/io/write_blif.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/views/names_view.hpp>

using namespace mockturtle;

namespace alice {

/********************************************************************
 * Genral stores                                                    *
 ********************************************************************/

/* aiger */
ALICE_ADD_STORE(aig_network, "aig", "a", "AIG", "AIGs")

ALICE_PRINT_STORE(aig_network, os, element) {
  os << "AIG PI/PO = " << element.num_pis() << "/" << element.num_pos() << "\n";
}

ALICE_DESCRIBE_STORE(aig_network, element) {
  return fmt::format("{} nodes", element.size());
}

/* genlib */
ALICE_ADD_STORE(std::vector<mockturtle::gate>, "genlib", "f", "GENLIB",
                "GENLIBs")

ALICE_PRINT_STORE(std::vector<mockturtle::gate>, os, element) {
  os << "GENLIB gate size = " << element.size() << "\n";
}

ALICE_DESCRIBE_STORE(std::vector<mockturtle::gate>, element) {
  return fmt::format("{} gates", element.size());
}

ALICE_ADD_FILE_TYPE(genlib, "Genlib");

ALICE_READ_FILE(std::vector<mockturtle::gate>, genlib, filename, cmd) {
  std::vector<mockturtle::gate> gates;
  if (lorina::read_genlib(filename, mockturtle::genlib_reader(gates)) !=
      lorina::return_code::success) {
    std::cout << "[w] parse error\n";
  }
  return gates;
}

ALICE_WRITE_FILE(std::vector<mockturtle::gate>, genlib, gates, filename, cmd) {
  std::cout << "[e] not supported" << std::endl;
}

ALICE_PRINT_STORE_STATISTICS(std::vector<mockturtle::gate>, os, gates) {
  os << fmt::format("Entered genlib library with {} gates", gates.size());
  os << "\n";
}

/********************************************************************
 * Read and Write                                                   *
 ********************************************************************/
ALICE_ADD_FILE_TYPE(aiger, "Aiger");

ALICE_READ_FILE(aig_network, aiger, filename, cmd) {
  aig_network aig;
  if (lorina::read_aiger(filename, mockturtle::aiger_reader(aig)) !=
      lorina::return_code::success) {
    std::cout << "[w] parse error\n";
  }
  return aig;
}

ALICE_PRINT_STORE_STATISTICS(aig_network, os, aig) {
  auto aig_copy = mockturtle::cleanup_dangling(aig);
  mockturtle::depth_view depth_aig{aig_copy};
  os << fmt::format("AIG   i/o = {}/{}   gates = {}   level = {}",
                    aig.num_pis(), aig.num_pos(), aig.num_gates(),
                    depth_aig.depth());
  os << "\n";
}

ALICE_ADD_FILE_TYPE(verilog, "Verilog");

ALICE_READ_FILE(aig_network, verilog, filename, cmd) {
  aig_network aig;

  if (lorina::read_verilog(filename, mockturtle::verilog_reader(aig)) !=
      lorina::return_code::success) {
    std::cout << "[w] parse error\n";
  }
  return aig;
}

/* show */
template <>
bool can_show<aig_network>(std::string &extension, command &cmd) {
  extension = "dot";

  return true;
}

template <>
void show<aig_network>(std::ostream &os, const aig_network &element,
                       const command &cmd) {
  gate_dot_drawer<aig_network> drawer;
  write_dot(element, os, drawer);
}



}  // namespace alice

#endif
