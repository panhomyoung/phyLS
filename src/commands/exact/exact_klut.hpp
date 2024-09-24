/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file exact_klut.hpp
 *
 * @brief STP based exact synthesis for k-feasible LUT
 *
 * @author Homyoung
 * @since  2023/12/19
 */

#ifndef EXACT_KLUT_HPP
#define EXACT_KLUT_HPP

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>

#include "../../core/exact/exact_lut.hpp"

using namespace std;
using namespace percy;
using namespace mockturtle;
using kitty::dynamic_truth_table;

namespace alice {
class exact_map_command : public command {
 public:
  explicit exact_map_command(const environment::ptr& env)
      : command(env, "exact synthesis to find optimal k-feasible LUTs") {
    add_option("function, -f", tt, "exact synthesis of function in hex");
    add_option("cut_size, -k", cut_size,
               "the number of LUT inputs form 2 to 8, default = 4");
    add_flag("--verbose, -v", "verbose results, default = true");
  }

 protected:
  string binary_to_hex() {
    string t_temp;
    for (int i = 0; i < tt.length(); i++) {
      switch (tt[i]) {
        case '0':
          t_temp += "0000";
          continue;
        case '1':
          t_temp += "0001";
          continue;
        case '2':
          t_temp += "0010";
          continue;
        case '3':
          t_temp += "0011";
          continue;
        case '4':
          t_temp += "0100";
          continue;
        case '5':
          t_temp += "0101";
          continue;
        case '6':
          t_temp += "0110";
          continue;
        case '7':
          t_temp += "0111";
          continue;
        case '8':
          t_temp += "1000";
          continue;
        case '9':
          t_temp += "1001";
          continue;
        case 'a':
          t_temp += "1010";
          continue;
        case 'b':
          t_temp += "1011";
          continue;
        case 'c':
          t_temp += "1100";
          continue;
        case 'd':
          t_temp += "1101";
          continue;
        case 'e':
          t_temp += "1110";
          continue;
        case 'f':
          t_temp += "1111";
          continue;
        case 'A':
          t_temp += "1010";
          continue;
        case 'B':
          t_temp += "1011";
          continue;
        case 'C':
          t_temp += "1100";
          continue;
        case 'D':
          t_temp += "1101";
          continue;
        case 'E':
          t_temp += "1110";
          continue;
        case 'F':
          t_temp += "1111";
          continue;
      }
    }
    return t_temp;
  }

  void execute() {
    t.clear();
    t.push_back(binary_to_hex());
    vector<string>& tt_h = t;

    int nr_in = 0, value = 0;
    while (value < tt.size()) {
      value = pow(2, nr_in);
      if (value == tt.size()) break;
      nr_in++;
    }
    nr_in += 2;

    cout << "Running exact LUT mapping for " << nr_in << "-input function."
         << endl;

    stopwatch<>::duration time{0};

    int count = 0;
    call_with_stopwatch(time, [&]() {
    //   phyLS::exact_lut_mapping(tt_h, nr_in, cut_size);
    phyLS::exact_lut_mapping(tt_h, nr_in, cut_size);
    for (auto x : tt_h) {
      if (!is_set("verbose")) cout << x << endl;
      count += 1;
    }
    });
    cout << "[LUTs number]: " << count << endl;
    // matrix factorization
    std::cout << fmt::format("[CPU time]: {:5.3f} seconds\n",
                             to_seconds(time));
  }

 private:
  string tt;
  vector<string> t;
  int cut_size = 4;
};

ALICE_ADD_COMMAND(exact_map, "Mapping")
}  // namespace alice

#endif