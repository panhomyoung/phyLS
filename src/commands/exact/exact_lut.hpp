/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file exact_lut.hpp
 *
 * @brief STP based exact synthesis
 *
 * @author Homyoung
 * @since  2022/11/23
 */

#ifndef EXACTLUT_HPP
#define EXACTLUT_HPP

#include <time.h>

#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>
#include <percy/percy.hpp>

#include "../../core/exact/exact_dag.hpp"
#include "../../core/exact/exact_lut.hpp"

using namespace std;
using namespace percy;
using namespace mockturtle;
using kitty::dynamic_truth_table;

namespace alice {
class exact_command : public command {
 public:
  explicit exact_command(const environment::ptr& env)
      : command(env, "exact synthesis to find optimal 2-LUTs") {
    add_option("function, -f", tt, "exact synthesis of function in hex");
    add_flag("--cegar_dag, -c", "cegar encoding and partial DAG structure");
    add_flag("--stp_cegar_dag, -s",
             "stp based cegar encoding and partial DAG structure");
    add_flag("--dag_depth, -t",
             "cegar encoding and partial DAG structure for Delay");
    add_option("--output, -o", filename, "the verilog filename");
    add_flag("--enumeration, -e", "enumerate all solutions");
    add_flag("--new_entry, -w", "adds k-LUT store entry");
    add_flag("--map, -m", "ASIC map each k-LUT");
    add_flag("--aig, -a", "enable exact synthesis for AIG, default = false");
    add_flag("--xag, -g", "enable exact synthesis for XAG, default = false");
    add_flag("--npn, -n", "print result for NPN storing, default = false");
    add_flag("--depth, -d", "print the depth of each result, default = false");
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

  void enu_es(int nr_in, list<chain>& chains) {
    int node = nr_in - 1;
    bool target = false;
    while (true) {
      spec spec;
      bsat_wrapper solver;
      partial_dag_encoder encoder2(solver);
      encoder2.reset_sim_tts(nr_in);

      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      spec.verbosity = 0;

      kitty::dynamic_truth_table f(nr_in);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;

      auto dags = pd_generate_filter(node, nr_in);
      spec.preprocess();
      for (auto& dag : dags) {
        chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder2);
        if (status == success) {
          chains.push_back(c);
          target = true;
        }
      }
      if (target) break;
      node++;
    }
  }

  void es(int nr_in, chain& result) {
    int node = nr_in - 1;
    bool target = false;
    while (true) {
      spec spec;
      bsat_wrapper solver;
      partial_dag_encoder encoder2(solver);
      encoder2.reset_sim_tts(nr_in);

      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      spec.verbosity = 0;

      kitty::dynamic_truth_table f(nr_in);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;

      auto dags = pd_generate_filter(node, nr_in);
      spec.preprocess();
      for (auto& dag : dags) {
        chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder2);
        if (status == success) {
          result.copy(c);
          target = true;
          break;
        }
      }
      if (target) break;
      node++;
    }
  }

  void es_delay(int nr_in, list<chain>& chains) {
    int node = nr_in - 1, max_level = 0, count = 0;
    bool target = false;
    while (true) {
      spec spec;
      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      spec.verbosity = 0;
      bsat_wrapper solver;
      partial_dag_encoder encoder2(solver);
      encoder2.reset_sim_tts(nr_in);
      kitty::dynamic_truth_table f(nr_in);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;

      auto dags = pd_generate_filter(node, nr_in);
      spec.preprocess();
      for (auto dag : dags) {
        chain c;
        const auto status = pd_cegar_synthesize(spec, c, dag, solver, encoder2);
        if (status == success) {
          target = true;
          int level_temp = c.get_nr_level();
          if (max_level == 0) {
            chains.push_back(c);
            max_level = level_temp;
          } else if (level_temp == max_level) {
            chains.push_back(c);
          } else if (level_temp < max_level) {
            chains.clear();
            chains.push_back(c);
            max_level = level_temp;
          }
        }
      }
      if (target) break;
      node++;
    }
    while (true) {
      if (max_level == 2 || count > 2) break;
      bool target_count = false;
      spec spec;
      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      spec.verbosity = 0;
      bsat_wrapper solver;
      partial_dag_encoder encoder2(solver);
      encoder2.reset_sim_tts(nr_in);
      kitty::dynamic_truth_table f(nr_in);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;

      node++;
      auto dags = pd_generate_filter(node, nr_in);
      spec.preprocess();
      for (auto dag : dags) {
        if (dag.nr_level() > max_level) continue;
        chain c;
        const auto status = pd_cegar_synthesize(spec, c, dag, solver, encoder2);
        if (status == success) {
          int level_temp = c.get_nr_level();
          if (level_temp == max_level) {
            chains.push_back(c);
          } else if (level_temp < max_level) {
            max_level = level_temp;
            chains.clear();
            chains.push_back(c);
            target_count = true;
          }
        }
      }
      if (!target_count) count++;
    }
  }

  klut_network create_network(chain& c) {
    klut_network klut;
    c.store_bench(0);
    std::string filename = "r_0.bench";
    if (lorina::read_bench(filename, mockturtle::bench_reader(klut)) !=
        lorina::return_code::success) {
      std::cout << "[w] parse error\n";
    }
    return klut;
  }

  bool compare_min_area(double area_temp, double delay_temp, int gate) {
    bool target = false;
    if (min_area == 0.00 && min_delay == 0.00) {
      min_area = area_temp;
      min_delay = delay_temp;
      mapping_gate = gate;
      target = true;
    } else if (area_temp < min_area) {
      min_area = area_temp;
      min_delay = delay_temp;
      mapping_gate = gate;
      target = true;
    } else if (area_temp == min_area) {
      if (delay_temp < min_delay) {
        min_delay = delay_temp;
        mapping_gate = gate;
        target = true;
      }
    }
    return target;
  }

  bool compare_min_delay(double area_temp, double delay_temp, int gate) {
    bool target = false;
    if (min_area == 0.00 && min_delay == 0.00) {
      min_area = area_temp;
      min_delay = delay_temp;
      mapping_gate = gate;
      target = true;
    } else if (delay_temp < min_delay) {
      min_area = area_temp;
      min_delay = delay_temp;
      mapping_gate = gate;
      target = true;
    } else if (delay_temp == min_delay) {
      if (area_temp < min_area) {
        min_area = area_temp;
        mapping_gate = gate;
        target = true;
      }
    }
    return target;
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

    cout << "Running exact synthesis for " << nr_in << "-input function."
         << endl
         << endl;

    clock_t begin, end;
    double totalTime = 0.0;

    phyLS::exact_lut_params ps;

    if (is_set("verbose")) ps.verbose = false;
    if (is_set("aig")) ps.aig = true;
    if (is_set("xag")) ps.xag = true;
    if (is_set("npn")) ps.npn = true;
    if (is_set("depth")) ps.depth = true;
    if (is_set("map")) ps.map = true;

    if (is_set("cegar_dag")) {
      if (is_set("enumeration")) {
        begin = clock();
        list<chain> chains;
        enu_es(nr_in, chains);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        int count = 0;
        chain best_chain;
        for (auto x : chains) {
          count += 1;
          if (ps.verbose) {
            x.print_bench();
            if (ps.depth)
              std::cout << "level = " << x.get_nr_level() << std::endl;
          }
          if (ps.npn) {
            x.print_npn();
            if (ps.depth) {
              std::cout << ": l{" << x.get_nr_level() << "}" << std::endl;
            } else {
              std::cout << std::endl;
            }
          }
          if (is_set("map")) {
            std::vector<mockturtle::gate> gates =
                store<std::vector<mockturtle::gate>>().current();
            mockturtle::tech_library<5> lib(gates);
            mockturtle::map_params ps;
            mockturtle::map_stats st;
            klut_network klut = create_network(x);
            auto res = mockturtle::map(klut, lib, ps, &st);
            if (compare_min_area(st.area, st.delay, res.num_gates())) {
              if (is_set("output")) {
                write_verilog_with_binding(res, filename);
              }
              best_chain.copy(x);
            }
          }
        }
        if (is_set("map")) {
          best_chain.print_npn();
          if (ps.depth) {
            std::cout << ": l{" << best_chain.get_nr_level() << "}"
                      << std::endl;
          } else {
            std::cout << std::endl;
          }
          std::cout << "Mapped gates = " << mapping_gate
                    << ", area = " << min_area << ", delay = " << min_delay
                    << std::endl;
        }
        cout << "[Total realization]: " << count << endl;
      } else {
        begin = clock();
        chain chain;
        es(nr_in, chain);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        if (is_set("new_entry")) {
          klut_network klut = create_network(chain);
          store<klut_network>().extend();
          store<klut_network>().current() = klut;
        }
        if (ps.verbose) {
          chain.print_bench();
          if (ps.depth) {
            klut_network klut = create_network(chain);
            mockturtle::depth_view depth_lut{klut};
            std::cout << "level = " << depth_lut.depth() - 1 << std::endl;
          }
        }
        if (ps.npn) {
          chain.print_npn();
          if (ps.depth) {
            klut_network klut = create_network(chain);
            mockturtle::depth_view depth_lut{klut};
            std::cout << "l{" << depth_lut.depth() - 1 << "}" << std::endl;
          } else {
            std::cout << std::endl;
          }
        }
        if (is_set("map")) {
          std::vector<mockturtle::gate> gates =
              store<std::vector<mockturtle::gate>>().current();
          mockturtle::tech_library<5> lib(gates);
          mockturtle::map_params ps;
          mockturtle::map_stats st;
          klut_network klut = create_network(chain);
          auto res = mockturtle::map(klut, lib, ps, &st);
          compare_min_area(st.area, st.delay, res.num_gates());
          chain.print_npn();
          if (is_set("output")) {
            write_verilog_with_binding(res, filename);
          }
          if (is_set("depth")) {
            klut_network klut = create_network(chain);
            mockturtle::depth_view depth_lut{klut};
            std::cout << ": l{" << depth_lut.depth() - 1 << "}" << std::endl;
          } else {
            std::cout << std::endl;
          }
          std::cout << "Mapped gates = " << mapping_gate
                    << ", area = " << min_area << ", delay = " << min_delay
                    << std::endl;
        }
      }
    } else if (is_set("dag_depth")) {
      begin = clock();
      list<chain> chains;
      es_delay(nr_in, chains);
      end = clock();
      totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
      int count = 0;
      chain best_chain;
      for (auto x : chains) {
        count += 1;
        if (is_set("verbose")) {
          x.print_bench();
          if (ps.depth)
            std::cout << "level = " << x.get_nr_level() << std::endl;
        }
        if (is_set("npn")) {
          x.print_npn();
          if (ps.depth) {
            std::cout << ": l{" << x.get_nr_level() << "}" << std::endl;
          } else {
            std::cout << std::endl;
          }
        }
        if (is_set("map")) {
          std::vector<mockturtle::gate> gates =
              store<std::vector<mockturtle::gate>>().current();
          mockturtle::tech_library<5> lib(gates);
          mockturtle::map_params ps;
          mockturtle::map_stats st;
          klut_network klut = create_network(x);
          auto res = mockturtle::map(klut, lib, ps, &st);
          if (compare_min_delay(st.area, st.delay, res.num_gates()))
            best_chain.copy(x);
        }
      }
      if (is_set("map")) {
        best_chain.print_npn();
        if (ps.depth) {
          std::cout << ": l{" << best_chain.get_nr_level() << "}" << std::endl;
        } else {
          std::cout << std::endl;
        }
        std::cout << "Mapped gates = " << mapping_gate
                  << ", area = " << min_area << ", delay = " << min_delay
                  << std::endl;
      }
      cout << "[Total realization]: " << count << endl;
    } else if (is_set("stp_cegar_dag")) {
      if (is_set("map")){
        ps.gates = store<std::vector<mockturtle::gate>>().current();
        begin = clock();
        exact_lut_dag_enu_map(tt, nr_in, ps);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        std::cout << "Mapped gates = " << ps.mapping_gate
                  << ", area = " << ps.min_area << ", delay = " << ps.min_delay
                  << std::endl;
      } else if (is_set("enumeration")) {
        begin = clock();
        exact_lut_dag_enu(tt, nr_in, ps);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        cout << "[Total realization]: " << nr_in << endl;
      } else {
        begin = clock();
        exact_lut_dag(tt, nr_in, ps);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        cout << "[Total realization]: " << nr_in << endl;
      }
    } else {
      if (is_set("enumeration")) {
        begin = clock();
        phyLS::exact_lut_enu(tt_h, nr_in);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        int count = 0;
        for (auto x : tt_h) {
          if (ps.verbose) cout << x << endl;
          count += 1;
        }
        cout << "[Total realization]: " << count << endl;
      } else {
        begin = clock();
        phyLS::exact_lut(tt_h, nr_in);
        end = clock();
        totalTime = (double)(end - begin) / CLOCKS_PER_SEC;
        int count = 0;
        for (auto x : tt_h) {
          if (ps.verbose) cout << x << endl;
          count += 1;
        }
        cout << "[Total realization]: " << count << endl;
      }
    }

    cout.setf(ios::fixed);
    cout << "[Total CPU time]   : " << setprecision(3) << totalTime << " s"
         << endl;
  }

 private:
  string tt;
  vector<string> t;
  double min_area = 0.00;
  double min_delay = 0.00;
  int mapping_gate = 0;
  std::string filename = "techmap.v";
};

ALICE_ADD_COMMAND(exact, "Synthesis")
}  // namespace alice

#endif