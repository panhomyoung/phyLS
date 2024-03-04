#ifndef EXACT_MULTI_HPP
#define EXACT_MULTI_HPP

#include <alice/alice.hpp>
#include <iostream>
#include <mockturtle/algorithms/klut_to_graph.hpp>
#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>
#include <string>
#include <vector>

#include "../store.hpp"

using namespace std;
using namespace percy;
using namespace mockturtle;
using kitty::dynamic_truth_table;

using namespace std;

namespace alice {

class exact_window_command : public command {
 public:
  explicit exact_window_command(const environment::ptr& env)
      : command(env, "using exact synthesis to find optimal window") {
    add_flag("--cegar, -c", "cegar encoding");
    add_option("--num_functions, -n", num_functions,
               "set the number of functions to be synthesized, default = 1");
    add_option("--file, -f", filename, "input filename");
    add_flag("--verbose, -v", "print the information");
  }

 private:
  int num_functions = 1;
  std::string filename;
  vector<string> iTT;

  aig_network create_network(chain& c) {
    klut_network klut;
    c.store_bench(0);
    std::string filename = "r_0.bench";
    if (lorina::read_bench(filename, mockturtle::bench_reader(klut)) !=
        lorina::return_code::success) {
      std::cout << "[w] parse error\n";
    }
    aig_network aig = convert_klut_to_graph<aig_network>(klut);
    return aig;
  }

 protected:
  void execute() {
    spec spec;
    aig_network aig;
    if (is_set("file")) {
      ifstream fin_bench(filename);
      string tmp;
      if (fin_bench.is_open()) {
        while (getline(fin_bench, tmp)) {
          iTT.push_back(tmp);
        }
        fin_bench.close();
        int nr_in = 0, value = 0;
        while (value < iTT[0].size()) {
          value = pow(2, nr_in);
          if (value == iTT[0].size()) break;
          nr_in++;
        }
        for (int i = 0; i < iTT.size(); i++) {
          kitty::dynamic_truth_table f(nr_in);
          kitty::create_from_binary_string(f, iTT[i]);
          spec[i] = f;
        }
      }
    } else {
      auto store_size = store<optimum_network>().size();
      assert(store_size >= num_functions);
      if (!is_set("num_functions")) num_functions = 1;
      for (int i = 0; i < num_functions; i++) {
        auto& opt = store<optimum_network>()[store_size - i - 1];
        auto copy = opt.function;
        spec[i] = copy;
      }
    }

    stopwatch<>::duration time{0};
    if (is_set("cegar")) {
      bsat_wrapper solver;
      msv_encoder encoder(solver);
      chain c;
      call_with_stopwatch(time, [&]() {
        if (synthesize(spec, c, solver, encoder) == success) {
          c.print_bench();
          aig = create_network(c);
          store<aig_network>().extend();
          store<aig_network>().current() = aig;
        }
      });
    } else {
      bsat_wrapper solver;
      ssv_encoder encoder(solver);
      chain c;
      call_with_stopwatch(time, [&]() {
        if (synthesize(spec, c, solver, encoder) == success) {
          c.print_bench();
          aig = create_network(c);
          store<aig_network>().extend();
          store<aig_network>().current() = aig;
        }
      });
    }

    std::cout << fmt::format("[Total CPU time]   :  {:5.3f} seconds\n",
                             to_seconds(time));
  }
};

ALICE_ADD_COMMAND(exact_window, "Synthesis")
}  // namespace alice

#endif
