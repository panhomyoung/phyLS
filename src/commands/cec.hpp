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

#ifndef CEC_HPP
#define CEC_HPP

#include <mockturtle/algorithms/equivalence_checking.hpp>
#include <mockturtle/algorithms/miter.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <vector>

using namespace std;
using namespace mockturtle;

namespace alice {

class cec_command : public command {
 public:
  explicit cec_command(const environment::ptr& env)
      : command(env, "combinational equivalence checking for AIG network") {
    add_option("file_1, -a", filename_1, "the input file name");
    add_option("file_2, -b", filename_2, "the input file name");
    add_flag("--verbose, -v", "print the information");
  }

 protected:
  void execute() {
    clock_t begin, end;
    double totalTime;

    begin = clock();
    aig_network aig1, aig2;
    if ((lorina::read_aiger(filename_1, mockturtle::aiger_reader(aig1)) ==
         lorina::return_code::success) &&
        (lorina::read_aiger(filename_2, mockturtle::aiger_reader(aig2)) ==
         lorina::return_code::success)) {
      if (aig1.num_pis() != aig2.num_pis())
        std::cerr << "Networks have different number of primary inputs."
                  << std::endl
                  << "Miter computation has failed." << std::endl;
      else {
        const auto miter_ntk = *miter<aig_network>(aig1, aig2);
        equivalence_checking_stats st;
        const auto result = equivalence_checking(miter_ntk, {}, &st);
        if (result)
          std::cout << "Networks are equivalent." << std::endl;
        else
          std::cout << "Networks are NOT EQUIVALENT." << std::endl;
      }
    } else {
      std::cerr << "[w] parse error" << std::endl;
    }
    end = clock();
    totalTime = (double)(end - begin) / CLOCKS_PER_SEC;

    cout.setf(ios::fixed);
    cout << "[CPU time]   " << setprecision(2) << totalTime << " s" << endl;
  }

 private:
  string filename_1;
  string filename_2;
};

ALICE_ADD_COMMAND(cec, "Verification")

}  // namespace alice

#endif