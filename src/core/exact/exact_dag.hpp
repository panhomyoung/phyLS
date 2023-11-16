/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2023 */

/**
 * @file exact_dag.hpp
 *
 * @brief dag generate
 *
 * @author Homyoung
 * @since  0.1
 */

#ifndef EXACT_DAG_HPP
#define EXACT_DAG_HPP

#include <math.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <kitty/kitty.hpp>
#include <mockturtle/mockturtle.hpp>
#include <mockturtle/networks/klut.hpp>
#include <numeric>
#include <optional>
#include <percy/percy.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace percy;
using namespace mockturtle;
using kitty::dynamic_truth_table;
using namespace std;

namespace phyLS {

template <typename T>
struct CollectionReverse {
  using iterator = typename T::reverse_iterator;
  using const_iterator = typename T::const_reverse_iterator;

  explicit CollectionReverse(T& col) : _M_collection(col) {}

  iterator begin() { return _M_collection.rbegin(); }

  const_iterator begin() const { return _M_collection.rbegin(); }

  iterator end() { return _M_collection.rend(); }

  const_iterator end() const { return _M_collection.rend(); }

 private:
  T& _M_collection;
};

template <typename T>
CollectionReverse<T> reverse(T& col) {
  return CollectionReverse<T>(col);
}

struct exact_lut_params {
  /*! \brief Enable exact synthesis for AIG. */
  bool aig{false};

  /*! \brief Enable exact synthesis for XAG. */
  bool xag{false};

  /*! \brief Print result for NPN storing. */
  bool npn{false};

  /*! \brief Print depth of each result. */
  bool depth{false};

  /*! \brief ASIC map each k-LUT. */
  bool map{false};
  std::vector<mockturtle::gate> gates;
  double min_area = 0.00;
  double min_delay = 0.00;
  int mapping_gate = 0;

  /*! \brief Be verbose. */
  bool verbose{true};
};

struct node_inf {
  int num;
  int level;
  bool fan_out = 0;
  bool fan_in = 0;
};

struct bench {
  int node;
  int right = 0;
  int left = 0;
  std::string tt = "2222";
};

struct dag {
  int node;
  int level;
  vector<int> count;
  vector<vector<node_inf>> lut;
};

struct matrix {
  int eye = 0;
  int node = 0;
  std::string name;
  bool input = 0;
};

struct result_lut {
  std::string computed_input;
  vector<bench> possible_result;
};

struct coordinate_dag {
  int Abscissa;
  int Ordinate;
  int parameter_Intermediate;
  int parameter_Gate;
};

struct cdccl_impl_dag {
  vector<std::string> Intermediate;
  std::string Result;
  vector<int> Gate;
  vector<coordinate_dag>
      Level;  // Define the network as a coordinate_dag system
};

class dag_impl {
 public:
  dag_impl(std::string& tt, int& input, exact_lut_params& ps)
      : tt(tt), input(input), ps(ps) {}

  vector<vector<bench>> exact_synthesis_results;

  void run() { stp_exact_synthesis(); }

  void run_enu() { stp_exact_synthesis_enu(); }

  void run_enu_map() { stp_exact_synthesis_enu_map(); }

  void run_rewrite() { stp_exact_synthesis_enu_for_rewrite(); }

  void run_rewrite_enu() { stp_exact_synthesis_enu_for_rewrite_enu(); }

 private:
  void stp_exact_synthesis() {
    int node = input - 1;
    bool target = false;
    while (true) {
      percy::spec spec;
      bsat_wrapper solver;
      partial_dag_encoder encoder(solver);
      encoder.reset_sim_tts(input);

      kitty::dynamic_truth_table f(input);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;
      spec.preprocess();

      auto dags = pd_generate_filter(node, input);
      int count = 0;
      for (auto& dag : dags) {
        percy::chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder);
        if (status == success) {
          vector<bench> lut = chain_to_bench(c);
          vector<vector<bench>> luts;
          luts.push_back(lut);
          stp_simulate(luts);
          if (luts.size()) {
            exact_synthesis_results.assign(luts.begin(), luts.end());
            if (ps.verbose) print_chain();
            if (ps.npn) print_npn();
            count += exact_synthesis_results.size();
            target = true;
            break;
          }
        }
      }
      if (target) {
        input = count;
        break;
      }
      node++;
    }
  }

  void stp_exact_synthesis_enu() {
    int node = input - 1;
    bool target = false;
    while (true) {
      percy::spec spec;
      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      bsat_wrapper solver;
      partial_dag_encoder encoder(solver);
      encoder.reset_sim_tts(input);
      kitty::dynamic_truth_table f(input);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;
      spec.preprocess();

      auto dags = pd_generate_filter(node, input);
      int count = 0;
      for (auto& dag : dags) {
        exact_synthesis_results.clear();
        percy::chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder);
        if (status == success) {
          vector<bench> lut = chain_to_bench(c);
          vector<vector<bench>> luts;
          luts.push_back(lut);
          stp_simulate(luts);
          if (luts.size()) {
            exact_synthesis_results.assign(luts.begin(), luts.end());
            if (ps.verbose) print_chain();
            if (ps.npn) print_npn();
            target = true;
            count += exact_synthesis_results.size();
          }
        }
      }
      if (target) {
        input = count;
        break;
      }
      node++;
    }
  }

  void stp_exact_synthesis_enu_map() {
    int node = input - 1;
    bool target = false;
    chain best_chain;
    while (true) {
      percy::spec spec;
      spec.add_alonce_clauses = false;
      spec.add_nontriv_clauses = false;
      spec.add_lex_func_clauses = false;
      spec.add_colex_clauses = false;
      spec.add_noreapply_clauses = false;
      spec.add_symvar_clauses = false;
      bsat_wrapper solver;
      partial_dag_encoder encoder(solver);
      encoder.reset_sim_tts(input);
      kitty::dynamic_truth_table f(input);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;
      spec.preprocess();

      auto dags = pd_generate_filter(node, input);
      for (auto dag : dags) {
        chain c;
        synth_result status =
            pd_cegar_synthesize(spec, c, dag, solver, encoder);
        if (status == success) {
          target = true;
          std::vector<mockturtle::gate> gates = ps.gates;
          mockturtle::tech_library<5> lib(gates);
          mockturtle::map_params pss;
          mockturtle::map_stats st;
          klut_network klut = create_network(c);
          auto res = mockturtle::map(klut, lib, pss, &st);
          if (compare_min_area(st.area, st.delay, res.num_gates()))
            best_chain.copy(c);
        }
      }
      if (target) {
        vector<bench> lut = chain_to_bench(best_chain);
        vector<vector<bench>> luts;
        luts.push_back(lut);
        stp_simulate(luts);
        if (luts.size())
          exact_synthesis_results.assign(luts.begin(), luts.end());
        break;
      }
      node++;
    }
    store_bench();
    vector<bench> exact_synthesis_result;
    for (int i = 0; i < exact_synthesis_results.size(); i++) {
      std::vector<mockturtle::gate> gates = ps.gates;
      mockturtle::tech_library<5> lib(gates);
      mockturtle::map_params ps;
      mockturtle::map_stats st;
      mockturtle::klut_network klut;
      std::string filename = "r_" + std::to_string(i) + ".bench";
      if (lorina::read_bench(filename, mockturtle::bench_reader(klut)) !=
          lorina::return_code::success) {
        std::cout << "[w] parse error\n";
      }
      auto res = mockturtle::map(klut, lib, ps, &st);
      if (compare_min_area(st.area, st.delay, res.num_gates()))
        exact_synthesis_result = exact_synthesis_results[i];
    }
    if (exact_synthesis_result.size()) {
      for (auto x : exact_synthesis_result) {
        cout << x.node << "-" << BinToHex(x.tt) << "-" << x.left << "-"
             << x.right << " ";
      }
    } else {
      best_chain.print_npn();
    }
    std::cout << ": l{" << best_chain.get_nr_level() << "}" << std::endl;
    ps.mapping_gate = mapping_gate;
    ps.min_area = min_area;
    ps.min_delay = min_delay;
  }

  void stp_exact_synthesis_enu_for_rewrite() {
    int node = input - 1;
    bool target = false;
    while (true) {
      percy::spec spec;
      bsat_wrapper solver;
      partial_dag_encoder encoder(solver);
      encoder.reset_sim_tts(input);

      kitty::dynamic_truth_table f(input);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;
      spec.preprocess();

      auto dags = pd_generate_filter(node, input);
      for (auto& dag : dags) {
        percy::chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder);
        if (status == success) {
          vector<bench> lut = chain_to_bench(c);
          vector<vector<bench>> luts;
          luts.push_back(lut);
          stp_simulate(luts);
          if (luts.size()) {
            exact_synthesis_results.assign(luts.begin(), luts.end());
            target = true;
            break;
          }
        }
      }
      if (target) break;
      node++;
    }
  }

  void stp_exact_synthesis_enu_for_rewrite_enu() {
    int node = input - 1;
    bool target = false;
    while (true) {
      percy::spec spec;
      bsat_wrapper solver;
      partial_dag_encoder encoder(solver);
      encoder.reset_sim_tts(input);

      kitty::dynamic_truth_table f(input);
      kitty::create_from_hex_string(f, tt);
      spec[0] = f;
      spec.preprocess();

      auto dags = pd_generate_filter(node, input);
      exact_synthesis_results.clear();
      for (auto& dag : dags) {
        percy::chain c;
        synth_result status;
        status = pd_cegar_synthesize(spec, c, dag, solver, encoder);
        if (status == success) {
          vector<bench> lut = chain_to_bench(c);
          vector<vector<bench>> luts;
          luts.push_back(lut);
          stp_simulate(luts);
          if (luts.size()) {
            exact_synthesis_results.insert(exact_synthesis_results.end(),
                                           luts.begin(), luts.end());
            target = true;
          }
        }
      }
      if (target) break;
      node++;
    }
  }

  vector<bench> chain_to_bench(percy::chain c) {
    vector<bench> results;
    vector<int> node, left, right;
    c.bench_infor(node, left, right);
    for (int i = node.size() - 1; i >= 0; i--) {
      bench result;
      result.node = node[i];
      result.left = left[i];
      result.right = right[i];
      results.push_back(result);
    }
    return results;
  }

  void stp_simulate(vector<vector<bench>>& lut_dags) {
    std::string tt_binary = hex_to_binary();
    int flag_node = lut_dags[0][lut_dags[0].size() - 1].node;
    vector<vector<bench>> lut_dags_new;
    for (int i = lut_dags.size() - 1; i >= 0; i--) {
      std::string input_tt(tt_binary);

      vector<matrix> matrix_form = chain_to_matrix(lut_dags[i], flag_node);
      matrix_computution(matrix_form);

      vector<result_lut> bench_result;
      result_lut first_result;
      first_result.computed_input = input_tt;
      first_result.possible_result = lut_dags[i];
      bench_result.push_back(first_result);
      for (int l = matrix_form.size() - 1; l >= 1; l--) {
        int length_string = input_tt.size();
        if (matrix_form[l].input == 0) {
          if (matrix_form[l].name == "W") {
            if (matrix_form[l].eye == 0) {
              std::string temp1, temp2;
              temp1 = input_tt.substr(length_string / 4, length_string / 4);
              temp2 = input_tt.substr(length_string / 2, length_string / 4);
              input_tt.replace(length_string / 4, length_string / 4, temp2);
              input_tt.replace(length_string / 2, length_string / 4, temp1);
            } else {
              int length2 = length_string / pow(2.0, matrix_form[l].eye + 2);
              for (int l1 = pow(2.0, matrix_form[l].eye), add = 1; l1 > 0;
                   l1--) {
                std::string temp1, temp2;
                temp1 = input_tt.substr(
                    (length_string / pow(2.0, matrix_form[l].eye + 1)) * add -
                        length2,
                    length2);
                temp2 = input_tt.substr(
                    (length_string / pow(2.0, matrix_form[l].eye + 1)) * add,
                    length2);
                input_tt.replace(
                    (length_string / pow(2.0, matrix_form[l].eye + 1)) * add -
                        length2,
                    length2, temp2);
                input_tt.replace(
                    (length_string / pow(2.0, matrix_form[l].eye + 1)) * add,
                    length2, temp1);
                add += 2;
              }
            }
            bench_result[0].computed_input = input_tt;
          } else if (matrix_form[l].name == "R") {
            if (matrix_form[l].eye == 0) {
              std::string temp(length_string, '2');
              input_tt.insert(input_tt.begin() + (length_string / 2),
                              temp.begin(), temp.end());
            } else {
              std::string temp(length_string / pow(2.0, matrix_form[l].eye),
                               '2');
              for (int l2 = pow(2.0, matrix_form[l].eye),
                       add1 = pow(2.0, matrix_form[l].eye + 1) - 1;
                   l2 > 0; l2--) {
                input_tt.insert(
                    input_tt.begin() +
                        ((length_string / pow(2.0, matrix_form[l].eye + 1)) *
                         add1),
                    temp.begin(), temp.end());
                add1 -= 2;
              }
            }
            bench_result[0].computed_input = input_tt;
          } else if (matrix_form[l].name == "M") {
            vector<result_lut> bench_result_temp;
            for (int q = bench_result.size() - 1; q >= 0; q--) {
              int length_string2 = bench_result[q].computed_input.size();
              int length1 = length_string2 /
                            pow(2.0, matrix_form[l].eye + 2);  // abcd的长度
              std::string standard(length1, '2');
              vector<int> standard_int(4, 0);
              vector<vector<int>> result;

              for (int l3 = 0; l3 < pow(2.0, matrix_form[l].eye); l3++) {
                vector<vector<int>> result_t;

                int ind = (length_string2 / pow(2.0, matrix_form[l].eye)) * l3;
                std::string a =
                    bench_result[q].computed_input.substr(ind, length1);
                std::string b = bench_result[q].computed_input.substr(
                    ind + length1, length1);
                std::string c = bench_result[q].computed_input.substr(
                    ind + (2 * length1), length1);
                std::string d = bench_result[q].computed_input.substr(
                    ind + (3 * length1), length1);

                if (a != standard) {
                  vector<int> result_temp(4);  // size为4的可能结果
                  result_temp[0] = 0;          // a=0
                  if (compare_string(a, b))    // b=a
                  {
                    if (b == standard)
                      result_temp[1] = 2;
                    else
                      result_temp[1] = 0;  // b=0
                    if (compare_string(a, c) &&
                        compare_string(b, c))  // c=(b=a)
                    {
                      if (c == standard)
                        result_temp[2] = 2;
                      else
                        result_temp[2] = 0;  // c=0
                      if (compare_string(a, d) && compare_string(b, d) &&
                          compare_string(c, d))  // d=(c=b=a)
                      {
                        result_temp[0] = 2;
                        result_temp[1] = 2;
                        result_temp[2] = 2;
                        result_temp[3] = 2;
                      } else  // d!=(c=b=a)
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(b, d)) result_temp[1] = 2;
                        if (compare_string(c, d)) result_temp[2] = 2;
                      }
                    } else  // c!=(b=a)
                    {
                      result_temp[2] = 1;  // c=1
                      if (compare_string(b, c)) result_temp[1] = 2;
                      if (compare_string(a, d) &&
                          compare_string(b, d))  // d=(b=a)
                      {
                        if (d == standard || compare_string(c, d))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(c, d))  // d=c
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(b, d)) result_temp[1] = 2;
                      } else  // 其他
                        break;
                    }
                  } else  // b!=a
                  {
                    result_temp[1] = 1;        // b=1
                    if (compare_string(a, c))  // c=a
                    {
                      if (c == standard || compare_string(b, c))
                        result_temp[2] = 2;
                      else
                        result_temp[2] = 0;  // c=0
                      if (compare_string(a, d) &&
                          compare_string(c, d))  // d=(c=a)
                      {
                        if (d == standard || compare_string(b, d))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(b, d))  // d=b
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else if (compare_string(b, c))  // c=b
                    {
                      result_temp[2] = 1;        // c=1
                      if (compare_string(a, d))  // d=a
                      {
                        if (d == standard ||
                            (compare_string(b, d) && compare_string(c, d)))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;  // d=0
                      } else if (compare_string(b, d) &&
                                 compare_string(c, d))  // d=(c=b)
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else  // 其他
                      break;
                  }
                  if (result_t.empty())
                    vector_generate(result_temp, result_t);
                  else {
                    vector<vector<int>> result_t_temp;
                    vector_generate(result_temp, result_t_temp);
                    for (int j = result_t.size() - 1; j >= 0; j--) {
                      for (int k = result_t_temp.size() - 1; k >= 0; k--) {
                        if (compare_vector(result_t[j], result_t_temp[k]))
                          result_t_temp.erase(result_t_temp.begin() + k);
                      }
                    }
                    if (!result_t_temp.empty()) {
                      result_t.insert(result_t.end(), result_t_temp.begin(),
                                      result_t_temp.end());
                    }
                  }
                }
                if (b != standard) {
                  vector<int> result_temp(4);  // size为4的可能结果
                  result_temp[1] = 0;          // b=0
                  if (compare_string(a, b))    // a=b
                  {
                    if (a == standard)
                      result_temp[0] = 2;
                    else
                      result_temp[0] = 0;  // a=0
                    if (compare_string(a, c) &&
                        compare_string(b, c))  // c=(a=b)
                    {
                      if (c == standard)
                        result_temp[2] = 2;
                      else
                        result_temp[2] = 0;  // c=0
                      if (compare_string(a, d) && compare_string(b, d) &&
                          compare_string(c, d))  // d=(c=a=b)
                      {
                        result_temp[0] = 2;
                        result_temp[1] = 2;
                        result_temp[2] = 2;
                        result_temp[3] = 2;
                      } else  // d!=(c=a=b)
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(a, d)) result_temp[0] = 2;
                        if (compare_string(c, d)) result_temp[2] = 2;
                      }
                    } else  // c!=(a=b)
                    {
                      result_temp[2] = 1;  // c=1
                      if (compare_string(a, c)) result_temp[0] = 2;
                      if (compare_string(a, d) &&
                          compare_string(b, d))  // d=(a=b)
                      {
                        if (d == standard || compare_string(c, d))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(c, d))  // d=c
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(a, d)) result_temp[0] = 2;
                      } else  // 其他
                        break;
                    }
                  } else  // a!=b
                  {
                    result_temp[0] = 1;        // a=1
                    if (compare_string(c, b))  // c=b
                    {
                      if (c == standard || compare_string(a, c))
                        result_temp[2] = 2;
                      else
                        result_temp[2] = 0;  // c=0
                      if (compare_string(b, d) &&
                          compare_string(c, d))  // d=(c=b)
                      {
                        if (d == standard || compare_string(a, d))
                          result_temp[3] = 2;  // d=0
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(a, d))  // d=a
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else if (compare_string(a, c))  // c=a
                    {
                      result_temp[2] = 1;        // c=1
                      if (compare_string(b, d))  // d=b
                      {
                        if (d == standard ||
                            (compare_string(a, d) && compare_string(c, d)))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;  // d=0
                      } else if (compare_string(a, d) &&
                                 compare_string(c, d))  // d=(c=a)
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else  // 其他
                      break;
                  }
                  if (result_t.empty())
                    vector_generate(result_temp, result_t);
                  else {
                    vector<vector<int>> result_t_temp;
                    vector_generate(result_temp, result_t_temp);
                    for (int j = result_t.size() - 1; j >= 0; j--) {
                      for (int k = result_t_temp.size() - 1; k >= 0; k--) {
                        if (compare_vector(result_t[j], result_t_temp[k]))
                          result_t_temp.erase(result_t_temp.begin() + k);
                      }
                    }
                    if (!result_t_temp.empty()) {
                      result_t.insert(result_t.end(), result_t_temp.begin(),
                                      result_t_temp.end());
                    }
                  }
                }
                if (c != standard) {
                  vector<int> result_temp(4);  // size为4的可能结果
                  result_temp[2] = 0;          // c=0
                  if (compare_string(a, c))    // a=c
                  {
                    if (a == standard)
                      result_temp[0] = 2;
                    else
                      result_temp[0] = 0;  // a=0
                    if (compare_string(b, c) &&
                        compare_string(b, a))  // b=(a=c)
                    {
                      if (b == standard)
                        result_temp[1] = 2;
                      else
                        result_temp[1] = 0;  // b=0
                      if (compare_string(a, d) && compare_string(b, d) &&
                          compare_string(c, d))  // d=(b=a=c)
                      {
                        result_temp[0] = 2;
                        result_temp[1] = 2;
                        result_temp[2] = 2;
                        result_temp[3] = 2;
                      } else  // d!=(b=a=c)
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(a, d)) result_temp[0] = 2;
                        if (compare_string(b, d)) result_temp[1] = 2;
                      }
                    } else  // b!=(a=c)
                    {
                      result_temp[1] = 1;  // b=1
                      if (compare_string(a, b)) result_temp[0] = 2;
                      if (compare_string(a, d) &&
                          compare_string(c, d))  // d=(a=c)
                      {
                        if (d == standard || compare_string(b, d))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(b, d))  // d=b
                      {
                        result_temp[3] = 1;  // d=1
                        if (compare_string(a, d)) result_temp[0] = 2;
                      } else  // 其他
                        break;
                    }
                  } else  // a!=c
                  {
                    result_temp[0] = 1;        // a=1
                    if (compare_string(b, c))  // b=c
                    {
                      if (b == standard || compare_string(b, a))
                        result_temp[1] = 2;
                      else
                        result_temp[1] = 0;  // b=0
                      if (compare_string(b, d) &&
                          compare_string(c, d))  // d=(b=c)
                      {
                        if (d == standard || compare_string(a, d))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;           // d=0
                      } else if (compare_string(a, d))  // d=a
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else if (compare_string(a, b))  // b=a
                    {
                      result_temp[1] = 1;        // b=1
                      if (compare_string(c, d))  // d=c
                      {
                        if (d == standard ||
                            (compare_string(a, d) && compare_string(b, d)))
                          result_temp[3] = 2;
                        else
                          result_temp[3] = 0;  // d=0
                      } else if (compare_string(a, d) &&
                                 compare_string(b, d))  // d=(b=a)
                        result_temp[3] = 1;             // d=1
                      else                              // 其他
                        break;
                    } else  // 其他
                      break;
                  }
                  if (result_t.empty())
                    vector_generate(result_temp, result_t);
                  else {
                    vector<vector<int>> result_t_temp;
                    vector_generate(result_temp, result_t_temp);
                    for (int j = result_t.size() - 1; j >= 0; j--) {
                      for (int k = result_t_temp.size() - 1; k >= 0; k--) {
                        if (compare_vector(result_t[j], result_t_temp[k]))
                          result_t_temp.erase(result_t_temp.begin() + k);
                      }
                    }
                    if (!result_t_temp.empty()) {
                      result_t.insert(result_t.end(), result_t_temp.begin(),
                                      result_t_temp.end());
                    }
                  }
                }
                if (d != standard) {
                  vector<int> result_temp(4);  // size为4的可能结果
                  result_temp[3] = 0;          // d=0
                  if (compare_string(a, d))    // a=d
                  {
                    if (a == standard)
                      result_temp[0] = 2;
                    else
                      result_temp[0] = 0;  // a=0
                    if (compare_string(b, a) &&
                        compare_string(b, d))  // b=(a=d)
                    {
                      if (b == standard)
                        result_temp[1] = 2;
                      else
                        result_temp[1] = 0;  // b=0
                      if (compare_string(a, c) && compare_string(b, c) &&
                          compare_string(d, c))  // c=(b=a=d)
                      {
                        result_temp[0] = 2;
                        result_temp[1] = 2;
                        result_temp[2] = 2;
                        result_temp[3] = 2;
                      } else  // c!=(b=a=d)
                      {
                        result_temp[2] = 1;  // c=1
                        if (compare_string(a, c)) result_temp[0] = 2;
                        if (compare_string(b, c)) result_temp[1] = 2;
                      }
                    } else  // b!=(a=d)
                    {
                      result_temp[1] = 1;  // b=1
                      if (compare_string(a, b)) result_temp[0] = 2;
                      if (compare_string(c, a) &&
                          compare_string(c, d))  // c=(a=d)
                      {
                        if (c == standard || compare_string(c, b))
                          result_temp[2] = 2;
                        else
                          result_temp[2] = 0;           // c=0
                      } else if (compare_string(c, b))  // c=b
                      {
                        result_temp[2] = 1;  // c=1
                        if (compare_string(a, c)) result_temp[0] = 2;
                      } else  // 其他
                        break;
                    }
                  } else  // a!=d
                  {
                    result_temp[0] = 1;        // a=1
                    if (compare_string(b, d))  // b=d
                    {
                      if (b == standard || compare_string(b, a))
                        result_temp[1] = 2;
                      else
                        result_temp[1] = 0;  // b=0
                      if (compare_string(c, d) &&
                          compare_string(c, b))  // c=(b=d)
                      {
                        if (c == standard || compare_string(c, a))
                          result_temp[2] = 2;
                        else
                          result_temp[2] = 0;           // c=0
                      } else if (compare_string(c, a))  // c=a
                        result_temp[2] = 1;             // c=1
                      else                              // 其他
                        break;
                    } else if (compare_string(b, a))  // b=a
                    {
                      result_temp[1] = 1;        // b=1
                      if (compare_string(c, d))  // c=d
                      {
                        if (c == standard ||
                            (compare_string(a, c) && compare_string(b, c)))
                          result_temp[2] = 2;
                        else
                          result_temp[2] = 0;  // c=0
                      } else if (compare_string(c, a) &&
                                 compare_string(c, b))  // c=(b=a)
                        result_temp[2] = 1;             // c=1
                      else                              // 其他
                        break;
                    } else  // 其他
                      break;
                  }
                  if (result_t.empty())
                    vector_generate(result_temp, result_t);
                  else {
                    vector<vector<int>> result_t_temp;
                    vector_generate(result_temp, result_t_temp);
                    for (int j = result_t.size() - 1; j >= 0; j--) {
                      for (int k = result_t_temp.size() - 1; k >= 0; k--) {
                        if (compare_vector(result_t[j], result_t_temp[k]))
                          result_t_temp.erase(result_t_temp.begin() + k);
                      }
                    }
                    if (!result_t_temp.empty()) {
                      result_t.insert(result_t.end(), result_t_temp.begin(),
                                      result_t_temp.end());
                    }
                  }
                }
                if (result.empty())
                  result.assign(result_t.begin(), result_t.end());
                else {
                  for (int j = result.size() - 1; j >= 0; j--) {
                    if (result[j] == standard_int) {
                      result.assign(result_t.begin(), result_t.end());
                      break;
                    } else {
                      bool target1 = 0, target2 = 0;
                      for (int k = result_t.size() - 1; k >= 0; k--) {
                        if (result_t[k] == standard_int) {
                          target1 = 1;
                          break;
                        } else {
                          if (compare_vector(result_t[k], result[j])) {
                            target2 = 1;
                            break;
                          }
                        }
                      }
                      if (target1) break;
                      if (!target2) result.erase(result.begin() + j);
                    }
                  }
                  if (result.empty()) break;
                }
              }
              if (result.empty()) {
                bench_result.erase(bench_result.begin() + q);
                continue;
              } else {
                for (int m = 0; m < result.size(); m++) {
                  std::string count1, count2;
                  std::string res1, res2;
                  for (int n = 0; n < result[m].size(); n++) {
                    if (result[m][n] == 0) {
                      count1 += "1";
                      count2 += "0";
                    } else {
                      count1 += "0";
                      count2 += "1";
                    }
                  }
                  if (count1 == "0000" || count1 == "1111" ||
                      count2 == "0000" || count2 == "1111")
                    continue;
                  for (int n = 0; n < pow(2.0, matrix_form[l].eye); n++) {
                    std::string s1(2 * length1, '2');
                    std::string s2(2 * length1, '2');
                    int ind =
                        (length_string2 / pow(2.0, matrix_form[l].eye)) * n;
                    std::string a, b, c, d;
                    a = bench_result[q].computed_input.substr(ind, length1);
                    b = bench_result[q].computed_input.substr(ind + length1,
                                                              length1);
                    c = bench_result[q].computed_input.substr(
                        ind + (length1 * 2), length1);
                    d = bench_result[q].computed_input.substr(
                        ind + (length1 * 3), length1);

                    if (count1[0] == '0') {
                      if (s1.substr(length1, length1) == standard)
                        s1.replace(length1, length1, a);
                    } else {
                      if (s1.substr(0, length1) == standard)
                        s1.replace(0, length1, a);
                    }
                    if (count1[1] == '0') {
                      if (s1.substr(length1, length1) == standard)
                        s1.replace(length1, length1, b);
                    } else {
                      if (s1.substr(0, length1) == standard)
                        s1.replace(0, length1, b);
                    }
                    if (count1[2] == '0') {
                      if (s1.substr(length1, length1) == standard)
                        s1.replace(length1, length1, c);
                    } else {
                      if (s1.substr(0, length1) == standard)
                        s1.replace(0, length1, c);
                    }
                    if (count1[3] == '0') {
                      if (s1.substr(length1, length1) == standard)
                        s1.replace(length1, length1, d);
                    } else {
                      if (s1.substr(0, length1) == standard)
                        s1.replace(0, length1, d);
                    }

                    if (count2[0] == '0') {
                      if (s2.substr(length1, length1) == standard)
                        s2.replace(length1, length1, a);
                    } else {
                      if (s2.substr(0, length1) == standard)
                        s2.replace(0, length1, a);
                    }
                    if (count2[1] == '0') {
                      if (s2.substr(length1, length1) == standard)
                        s2.replace(length1, length1, b);
                    } else {
                      if (s2.substr(0, length1) == standard)
                        s2.replace(0, length1, b);
                    }
                    if (count2[2] == '0') {
                      if (s2.substr(length1, length1) == standard)
                        s2.replace(length1, length1, c);
                    } else {
                      if (s2.substr(0, length1) == standard)
                        s2.replace(0, length1, c);
                    }
                    if (count2[3] == '0') {
                      if (s2.substr(length1, length1) == standard)
                        s2.replace(length1, length1, d);
                    } else {
                      if (s2.substr(0, length1) == standard)
                        s2.replace(0, length1, d);
                    }
                    res1 += s1;
                    res2 += s2;
                  }
                  result_lut result_temp_1, result_temp_2;
                  result_temp_1.possible_result =
                      bench_result[q].possible_result;
                  result_temp_2.possible_result =
                      bench_result[q].possible_result;
                  for (int p = 0; p < result_temp_1.possible_result.size();
                       p++) {
                    if (result_temp_1.possible_result[p].node ==
                        matrix_form[l].node)
                      result_temp_1.possible_result[p].tt = count1;
                    if (result_temp_2.possible_result[p].node ==
                        matrix_form[l].node)
                      result_temp_2.possible_result[p].tt = count2;
                  }
                  if (res1.size() == 4) {
                    for (int p = 0; p < result_temp_1.possible_result.size();
                         p++) {
                      if (result_temp_1.possible_result[p].node ==
                          matrix_form[0].node)
                        result_temp_1.possible_result[p].tt = res1;
                      if (result_temp_2.possible_result[p].node ==
                          matrix_form[0].node)
                        result_temp_2.possible_result[p].tt = res2;
                    }
                  } else {
                    result_temp_1.computed_input = res1;
                    result_temp_2.computed_input = res2;
                  }

                  bench_result_temp.push_back(result_temp_1);
                  bench_result_temp.push_back(result_temp_2);
                }
              }
            }
            if (bench_result_temp.empty())
              break;
            else
              bench_result.assign(bench_result_temp.begin(),
                                  bench_result_temp.end());
          }
        }
      }

      vector<result_lut> bench_result1;
      if (ps.aig) {
        for (auto x : bench_result) {
          bool aig_target = true;
          for (auto y : x.possible_result) {
            if (y.tt == "0001" || y.tt == "0010" || y.tt == "0100" ||
                y.tt == "0111" || y.tt == "1000" || y.tt == "1011" ||
                y.tt == "1101" || y.tt == "1110") {
              continue;
            } else {
              aig_target = false;
            }
          }
          if (aig_target) {
            bench_result1.push_back(x);
          }
        }
      } else if (ps.xag) {
        for (auto x : bench_result) {
          bool xag_target = true;
          for (auto y : x.possible_result) {
            if (y.tt == "0001" || y.tt == "0010" || y.tt == "0100" ||
                y.tt == "0111" || y.tt == "1000" || y.tt == "1011" ||
                y.tt == "1101" || y.tt == "1110" || y.tt == "0110" ||
                y.tt == "1001") {
              continue;
            } else {
              xag_target = false;
            }
          }
          if (xag_target) {
            bench_result1.push_back(x);
          }
        }
      } else {
        bench_result1 = bench_result;
      }

      for (int j = bench_result1.size() - 1; j >= 0; j--) {
        vector<vector<int>> mtxvec;
        vector<int> mtx1, mtx2;
        vector<std::string> in;
        for (int k = bench_result1[j].possible_result.size() - 1; k >= 0; k--) {
          vector<int> mtxvec_temp;
          in.push_back(bench_result1[j].possible_result[k].tt);
          mtxvec_temp.push_back(bench_result1[j].possible_result[k].left);
          mtxvec_temp.push_back(bench_result1[j].possible_result[k].right);
          mtxvec_temp.push_back(bench_result1[j].possible_result[k].node);
          mtxvec.push_back(mtxvec_temp);
        }
        in.push_back("10");
        mtx1.push_back(bench_result1[j].possible_result[0].node);
        mtx2.push_back(input);
        mtx2.push_back(1);
        mtxvec.push_back(mtx1);
        mtxvec.push_back(mtx2);
        vector<vector<std::string>> in_expansion = bench_expansion(in);
        vector<result_lut> bench_temp;
        for (int k = in_expansion.size() - 1; k >= 0; k--) {
          std::string tt_temp = in_expansion[k][in_expansion[k].size() - 2];
          bench_solve(in_expansion[k], mtxvec);
          if (!compare_result(in_expansion[k], tt_binary)) {
            in_expansion.erase(in_expansion.begin() + k);
          } else {
            result_lut bench_temp_temp = bench_result1[j];
            bench_temp_temp.possible_result[0].tt = tt_temp;
            bench_temp.push_back(bench_temp_temp);
          }
        }
        if (in_expansion.empty())
          bench_result1.erase(bench_result1.begin() + j);
        else {
          bench_result1.erase(bench_result1.begin() + j);
          bench_result1.insert(bench_result1.end(), bench_temp.begin(),
                               bench_temp.end());
        }
      }

      if (bench_result1.size()) {
        for (int j = 0; j < bench_result1.size(); j++)
          lut_dags_new.push_back(bench_result1[j].possible_result);
      }
    }
    lut_dags.assign(lut_dags_new.begin(), lut_dags_new.end());
  }

  std::string hex_to_binary() {
    std::string t_temp;
    for (int i = 0; i < tt.size(); i++) {
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

  std::string BinToHex(std::string& strBin) {
    std::string strHex;
    strHex.resize(strBin.size() / 4);
    if (strBin == "0000") {
      strHex = "0";
    } else if (strBin == "0001") {
      strHex = "1";
    } else if (strBin == "0010") {
      strHex = "2";
    } else if (strBin == "0011") {
      strHex = "3";
    } else if (strBin == "0100") {
      strHex = "4";
    } else if (strBin == "0101") {
      strHex = "5";
    } else if (strBin == "0110") {
      strHex = "6";
    } else if (strBin == "0111") {
      strHex = "7";
    } else if (strBin == "1000") {
      strHex = "8";
    } else if (strBin == "1001") {
      strHex = "9";
    } else if (strBin == "1010") {
      strHex = "a";
    } else if (strBin == "1011") {
      strHex = "b";
    } else if (strBin == "1100") {
      strHex = "c";
    } else if (strBin == "1101") {
      strHex = "d";
    } else if (strBin == "1110") {
      strHex = "e";
    } else {
      strHex = "f";
    }
    return strHex;
  }

  vector<matrix> chain_to_matrix(vector<bench> lut_dags, int flag_node) {
    vector<matrix> matrix_form;
    matrix temp_node, temp_left, temp_right;
    temp_node.node = lut_dags[0].node;
    temp_node.name = "M";
    temp_left.node = lut_dags[0].left;
    temp_right.node = lut_dags[0].right;
    matrix_form.push_back(temp_node);

    if (lut_dags[0].right >= flag_node) {
      vector<matrix> matrix_form_temp2;
      matrix_generate(lut_dags, matrix_form_temp2, lut_dags[0].right);
      matrix_form.insert(matrix_form.end(), matrix_form_temp2.begin(),
                         matrix_form_temp2.end());
    } else {
      temp_right.input = 1;
      std::string str = to_string(lut_dags[0].right);
      temp_right.name = str;
      matrix_form.push_back(temp_right);
    }

    if (lut_dags[0].left >= flag_node) {
      vector<matrix> matrix_form_temp1;
      matrix_generate(lut_dags, matrix_form_temp1, lut_dags[0].left);
      matrix_form.insert(matrix_form.end(), matrix_form_temp1.begin(),
                         matrix_form_temp1.end());
    } else {
      temp_left.input = 1;
      std::string str = to_string(lut_dags[0].left);
      temp_left.name = str;
      matrix_form.push_back(temp_left);
    }

    return matrix_form;
  }

  void matrix_computution(vector<matrix>& matrix_form) {
    bool status = true;
    while (status) {
      status = false;
      for (int i = matrix_form.size() - 1; i > 1; i--) {
        // 两个变量交换
        if (((matrix_form[i].name != "M") && (matrix_form[i].name != "R") &&
             (matrix_form[i].name != "W")) &&
            ((matrix_form[i - 1].name != "M") &&
             (matrix_form[i - 1].name != "R") &&
             (matrix_form[i - 1].name != "W"))) {
          int left, right;
          left = atoi(matrix_form[i - 1].name.c_str());
          right = atoi(matrix_form[i].name.c_str());
          // 相等变量降幂
          if (left == right) {
            status = true;
            matrix reduce_matrix;
            reduce_matrix.name = "R";
            matrix_form.insert(matrix_form.begin() + (i - 1), reduce_matrix);
            matrix_form.erase(matrix_form.begin() + i);
          } else if (left < right) {
            status = true;
            matrix swap_matrix;
            swap_matrix.name = "W";
            swap(matrix_form[i - 1], matrix_form[i]);
            matrix_form.insert(matrix_form.begin() + (i - 1), swap_matrix);
          }
        }
        // 变量与矩阵交换
        if (((matrix_form[i].name == "M") || (matrix_form[i].name == "R") ||
             (matrix_form[i].name == "W")) &&
            ((matrix_form[i - 1].name != "M") &&
             (matrix_form[i - 1].name != "R") &&
             (matrix_form[i - 1].name != "W"))) {
          status = true;
          matrix_form[i].eye += 1;
          swap(matrix_form[i - 1], matrix_form[i]);
        }
      }
    }
  }

  void matrix_generate(vector<bench> lut, vector<matrix>& matrix_form,
                       int node) {
    int flag = lut[lut.size() - 1].node;
    for (int i = 0; i < lut.size(); i++) {
      if (lut[i].node == node) {
        matrix temp_node, temp_left, temp_right;
        temp_node.node = node;
        temp_node.name = "M";
        matrix_form.push_back(temp_node);
        if (lut[i].right >= flag) {
          vector<matrix> matrix_form_temp2;
          matrix_generate(lut, matrix_form_temp2, lut[i].right);
          matrix_form.insert(matrix_form.end(), matrix_form_temp2.begin(),
                             matrix_form_temp2.end());
        } else {
          temp_right.input = 1;
          temp_right.node = lut[i].right;
          std::string str = to_string(lut[i].right);
          temp_right.name = str;
          matrix_form.push_back(temp_right);
        }
        if (lut[i].left >= flag) {
          vector<matrix> matrix_form_temp1;
          matrix_generate(lut, matrix_form_temp1, lut[i].left);
          matrix_form.insert(matrix_form.begin() + 1, matrix_form_temp1.begin(),
                             matrix_form_temp1.end());
        } else {
          temp_left.input = 1;
          temp_left.node = lut[i].left;
          std::string str = to_string(lut[i].left);
          temp_left.name = str;
          matrix_form.push_back(temp_left);
        }
      }
    }
  }

  bool compare_string(std::string a, std::string b) {
    bool target = true;
    for (int i = 0; i < a.size(); i++) {
      if ((a[i] != b[i]) && a[i] != '2' && b[i] != '2') {
        target = false;
        break;
      }
    }
    return target;
  }

  bool compare_vector(vector<int> a, vector<int> b) {
    bool target = 0;
    if (a == b) {
      target = 1;
    } else {
      for (int i = 0; i < a.size(); i++) {
        if (a[i] == 1) {
          a[i] = 0;
        } else if (a[i] == 0) {
          a[i] = 1;
        }
      }
      if (a == b) {
        target = 1;
      }
    }
    return target;
  }

  bool compare_result(vector<std::string> t1, std::string t2) {
    bool target = false;
    std::string t3(t2.size(), '0');
    for (int i = 0; i < t1.size(); i++) {
      char temp;
      for (int j = 0; j < t1[i].size() / 2; j++) {
        temp = t1[i][j];
        t1[i][j] = t1[i][t1[i].size() - 1 - j];
        t1[i][t1[i].size() - 1 - j] = temp;
      }
      vector<int> local;
      for (int j = 0; j < t1[i].size(); j++) {
        if (t1[i][j] == '2') local.push_back(j);
      }
      if (local.size()) {
        vector<std::string> t_temp;
        t_temp.push_back(t1[i]);
        for (auto x : local) {
          vector<std::string> t_temp_temp;
          for (auto y : t_temp) {
            string temp1 = y, temp2 = y;
            temp1[x] = '1';
            temp2[x] = '0';
            t_temp_temp.push_back(temp1);
            t_temp_temp.push_back(temp2);
          }
          t_temp = t_temp_temp;
        }
        for (auto x : t_temp) {
          int value = stoi(x, 0, 2);
          int ind = t3.size() - value - 1;
          t3[ind] = '1';
        }
      } else {
        int value = stoi(t1[i], 0, 2);
        int ind = t3.size() - value - 1;
        t3[ind] = '1';
      }
    }

    if (t2 == t3) target = true;
    return target;
  }

  void vector_generate(vector<int> result_b, vector<vector<int>>& result_a) {
    for (int i = 0; i < result_b.size(); i++) {
      if (result_b[i] != 2) {
        if (result_a.empty()) {
          vector<int> temp;
          temp.push_back(result_b[i]);
          result_a.push_back(temp);
        } else {
          vector<vector<int>> result_a_temp;
          for (int j = 0; j < result_a.size(); j++) {
            vector<int> temp(result_a[j]);
            temp.push_back(result_b[i]);
            result_a_temp.push_back(temp);
          }
          result_a.assign(result_a_temp.begin(), result_a_temp.end());
        }
      } else {
        if (result_a.empty()) {
          vector<int> temp1(1, 1);
          vector<int> temp2(1, 0);
          result_a.push_back(temp1);
          result_a.push_back(temp2);
        } else {
          vector<vector<int>> result_a_temp;
          for (int j = 0; j < result_a.size(); j++) {
            vector<int> temp1(result_a[j]);
            vector<int> temp2(result_a[j]);
            temp1.push_back(1);
            temp2.push_back(0);
            result_a_temp.push_back(temp1);
            result_a_temp.push_back(temp2);
          }
          result_a.assign(result_a_temp.begin(), result_a_temp.end());
        }
      }
    }
  }

  void print_chain() { to_chain(std::cout); }

  void to_chain(std::ostream& s) {
    for (int i = 0; i < exact_synthesis_results.size(); i++) {
      for (auto x : phyLS::reverse(exact_synthesis_results[i])) {
        s << x.node << " = 4'b" << x.tt << " (" << x.left << ", " << x.right
          << ")  ";
      }
      s << "\n";
    }
  }

  void print_npn() { to_npn(std::cout); }

  void to_npn(std::ostream& s) {
    for (int i = 0; i < exact_synthesis_results.size(); i++) {
      for (auto x : phyLS::reverse(exact_synthesis_results[i])) {
        s << x.node << "-" << BinToHex(x.tt) << "-" << x.left << "-" << x.right
          << " ";
      }
      s << "\n";
    }
  }

  void store_bench() {
    for (int i = 0; i < exact_synthesis_results.size(); i++) {
      std::ofstream file;
      std::string filename = "r_" + std::to_string(i) + ".bench";
      file.open(filename, std::ios::out);
      for (int i = 0; i < input; i++) {
        const auto idx = i + 1;
        file << "INPUT(n" << idx << ")\n";
      }
      file << "OUTPUT(po" << 0 << ")\n";
      file << "n0 = gnd\n";
      for (auto x : phyLS::reverse(exact_synthesis_results[i])) {
        file << "n" << x.node << " = ";
        file << "LUT 0x" << BinToHex(x.tt) << " (n" << x.left << ", n"
             << x.right << ")\n";
      }
      file << "po0 = LUT 0x2 (n" << exact_synthesis_results[i][0].node << ")\n";
      file.close();
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

  void bench_solve(vector<std::string>& in, vector<vector<int>> mtxvec) {
    int length1 = mtxvec[mtxvec.size() - 1][0];  // number of primary input
    int length2 = mtxvec[mtxvec.size() - 1][1];  // number of primary output
    int length3 = mtxvec[0][2];                  // the minimum minuend
    int length4 =
        mtxvec[mtxvec.size() - 2 - length2][2];  // the maximum variable
    vector<vector<cdccl_impl_dag>>
        list;  // the solution space is two dimension vector
    vector<cdccl_impl_dag> list_temp;
    cdccl_impl_dag list_temp_temp;

    std::string Result_temp(length1, '2');  // temporary result
    coordinate_dag Level_temp;
    Level_temp.Abscissa = -1;
    Level_temp.Ordinate = -1;
    Level_temp.parameter_Intermediate = -1;
    Level_temp.parameter_Gate = -1;
    list_temp_temp.Level.resize(
        length4, Level_temp);  // Initialize level as a space with the size of
                               // variables(length4)
    /*
     * initialization
     */
    coordinate_dag Gate_level;
    Gate_level.Abscissa = 0;
    Gate_level.Ordinate = 0;
    Gate_level.parameter_Intermediate = -1;
    Gate_level.parameter_Gate = -1;
    std::string Intermediate_temp;
    for (int i = mtxvec.size() - 1 - length2; i < mtxvec.size() - 1;
         i++)  // the original intermediate
    {
      if (mtxvec[i][0] < length3) {
        if (in[i][0] == '1')
          Result_temp[mtxvec[i][0] - 1] = '1';
        else
          Result_temp[mtxvec[i][0] - 1] = '0';
      } else {
        list_temp_temp.Gate.push_back(mtxvec[i][0]);
        if (in[i][0] == '1')
          Intermediate_temp += "1";
        else
          Intermediate_temp += "0";
      }
      list_temp_temp.Level[mtxvec[i][0] - 1] = Gate_level;
    }
    list_temp_temp.Result = Result_temp;
    list_temp_temp.Intermediate.push_back(Intermediate_temp);
    list_temp.push_back(list_temp_temp);  // level 0
    list.push_back(list_temp);
    /*
     * The first level information
     */
    int count1 = 0;
    for (int level = 0;; level++)  // the computation process
    {
      int flag = 0, ordinate = 0;  // the flag of the end of the loop & the
                                   // ordinate of each level's gate
      vector<cdccl_impl_dag> list_temp1;
      for (int k = 0; k < list[level].size(); k++) {
        cdccl_impl_dag temp1;                  // temporary gate
        temp1.Result = list[level][k].Result;  // first, next level's Result is
                                               // same as the front level
        for (int j = 0; j < list[level][k].Intermediate.size(); j++) {
          temp1.Level = list[level][k].Level;  // next level's Level information
          for (int j1 = 0; j1 < list[level][k].Gate.size(); j1++) {
            temp1.Level[list[level][k].Gate[j1] - 1].parameter_Intermediate = j;
            temp1.Level[list[level][k].Gate[j1] - 1].parameter_Gate = j1;
          }

          coordinate_dag level_current;  // new level
          level_current.Abscissa = level + 1;
          level_current.parameter_Intermediate = -1;
          level_current.parameter_Gate = -1;
          level_current.Ordinate = ordinate;

          temp1.Gate.assign(list[level][k].Gate.begin(),
                            list[level][k].Gate.end());  // need more!!!!!
          temp1.Intermediate.push_back(list[level][k].Intermediate[j]);

          vector<std::string> Intermediate_temp;
          vector<int> Gate_temp;
          vector<int> Gate_judge(length4, -1);
          int count_cdccl = 0;
          for (int i = 0; i < temp1.Gate.size(); i++) {
            int length = temp1.Gate[i] - length3;
            int Gate_f = mtxvec[length][0];  // the front Gate variable
            int Gate_b = mtxvec[length][1];  // the behind Gate variable
            std::string tt = in[length];     // the correndsponding Truth Table
            char target = temp1.Intermediate[0][i];  // the SAT target
            vector<std::string> Intermediate_temp_temp;

            int flag_cdccl = 0;
            std::string Intermediate_temp_temp_F, Intermediate_temp_temp_B;
            if (temp1.Level[Gate_f - 1].Abscissa >= 0) {
              if (Gate_f < length3) {
                char contrast_R1 = list[temp1.Level[Gate_f - 1].Abscissa]
                                       [temp1.Level[Gate_f - 1].Ordinate]
                                           .Result[Gate_f - 1];
                Intermediate_temp_temp_F.push_back(contrast_R1);
              } else {
                char contrast_I1 =
                    list[temp1.Level[Gate_f - 1].Abscissa]
                        [temp1.Level[Gate_f - 1].Ordinate]
                            .Intermediate
                                [temp1.Level[Gate_f - 1].parameter_Intermediate]
                                [temp1.Level[Gate_f - 1].parameter_Gate];
                Intermediate_temp_temp_F.push_back(contrast_I1);
              }
              flag_cdccl += 1;
            }
            if (temp1.Level[Gate_b - 1].Abscissa >= 0) {
              if (Gate_b < length3) {
                char contrast_R2 = list[temp1.Level[Gate_b - 1].Abscissa]
                                       [temp1.Level[Gate_b - 1].Ordinate]
                                           .Result[Gate_b - 1];
                Intermediate_temp_temp_B.push_back(contrast_R2);
              } else {
                char contrast_I2 =
                    list[temp1.Level[Gate_b - 1].Abscissa]
                        [temp1.Level[Gate_b - 1].Ordinate]
                            .Intermediate
                                [temp1.Level[Gate_b - 1].parameter_Intermediate]
                                [temp1.Level[Gate_b - 1].parameter_Gate];
                Intermediate_temp_temp_B.push_back(contrast_I2);
              }
              flag_cdccl += 2;
            }
            if (Intermediate_temp.size() == 0) {
              if (flag_cdccl == 0) {
                Gate_judge[Gate_f - 1] = count_cdccl;
                count_cdccl += 1;
                Gate_judge[Gate_b - 1] = count_cdccl;
                count_cdccl += 1;
                Gate_temp.push_back(Gate_f);
                Gate_temp.push_back(Gate_b);
                if (tt[0] == target) Intermediate_temp_temp.push_back("11");
                if (tt[1] == target) Intermediate_temp_temp.push_back("01");
                if (tt[2] == target) Intermediate_temp_temp.push_back("10");
                if (tt[3] == target) Intermediate_temp_temp.push_back("00");
              } else if (flag_cdccl == 1) {
                Gate_judge[Gate_b - 1] = count_cdccl;
                count_cdccl += 1;
                Gate_temp.push_back(Gate_b);
                if (tt[0] == target) {
                  if (Intermediate_temp_temp_F == "1")
                    Intermediate_temp_temp.push_back("1");
                }
                if (tt[1] == target) {
                  if (Intermediate_temp_temp_F == "0")
                    Intermediate_temp_temp.push_back("1");
                }
                if (tt[2] == target) {
                  if (Intermediate_temp_temp_F == "1")
                    Intermediate_temp_temp.push_back("0");
                }
                if (tt[3] == target) {
                  if (Intermediate_temp_temp_F == "0")
                    Intermediate_temp_temp.push_back("0");
                }
              } else if (flag_cdccl == 2) {
                Gate_judge[Gate_f - 1] = count_cdccl;
                count_cdccl += 1;
                Gate_temp.push_back(Gate_f);
                if (tt[0] == target) {
                  if (Intermediate_temp_temp_B == "1")
                    Intermediate_temp_temp.push_back("1");
                }
                if (tt[1] == target) {
                  if (Intermediate_temp_temp_B == "1")
                    Intermediate_temp_temp.push_back("0");
                }
                if (tt[2] == target) {
                  if (Intermediate_temp_temp_B == "0")
                    Intermediate_temp_temp.push_back("1");
                }
                if (tt[3] == target) {
                  if (Intermediate_temp_temp_B == "0")
                    Intermediate_temp_temp.push_back("0");
                }
              } else {
                int t0 = 0, t1 = 0, t2 = 0, t3 = 0;
                if (tt[0] == target) {
                  if ((Intermediate_temp_temp_F == "1") &&
                      (Intermediate_temp_temp_B == "1"))
                    t0 = 1;
                }
                if (tt[1] == target) {
                  if ((Intermediate_temp_temp_F == "0") &&
                      (Intermediate_temp_temp_B == "1"))
                    t1 = 1;
                }
                if (tt[2] == target) {
                  if ((Intermediate_temp_temp_F == "1") &&
                      (Intermediate_temp_temp_B == "0"))
                    t2 = 1;
                }
                if (tt[3] == target) {
                  if ((Intermediate_temp_temp_F == "0") &&
                      (Intermediate_temp_temp_B == "0"))
                    t3 = 1;
                }
                if ((t0 == 1) || (t1 == 1) || (t2 == 1) || (t3 == 1)) {
                  Gate_judge[Gate_f - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_judge[Gate_b - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_f);
                  Gate_temp.push_back(Gate_b);
                  if (t0 == 1) Intermediate_temp_temp.push_back("11");
                  if (t1 == 1) Intermediate_temp_temp.push_back("01");
                  if (t2 == 1) Intermediate_temp_temp.push_back("10");
                  if (t3 == 1) Intermediate_temp_temp.push_back("00");
                }
              }
            } else {
              if (flag_cdccl == 0) {
                int count_Gate_f = 0, count_Gate_b = 0;
                for (int j = 0; j < Intermediate_temp.size(); j++) {
                  int flag;
                  std::string t1, t2, t3, t4;
                  if (Gate_judge[Gate_f - 1] < 0) {
                    count_Gate_f = 1;
                    if (tt[0] == target) t1 = "1";
                    if (tt[1] == target) t2 = "0";
                    if (tt[2] == target) t3 = "1";
                    if (tt[3] == target) t4 = "0";
                    flag = 1;
                  } else {
                    int count_sat = 0;
                    if (tt[0] == target) {
                      if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1') {
                        t1 = "11";
                        count_sat += 1;
                      }
                    }
                    if (tt[1] == target) {
                      if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0') {
                        t2 = "01";
                        count_sat += 1;
                      }
                    }
                    if (tt[2] == target) {
                      if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1') {
                        t3 = "10";
                        count_sat += 1;
                      }
                    }
                    if (tt[3] == target) {
                      if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0') {
                        t4 = "00";
                        count_sat += 1;
                      }
                    }
                    if (count_sat == 0) continue;
                    flag = 2;
                  }
                  if (Gate_judge[Gate_b - 1] < 0) {
                    count_Gate_b = 1;
                    if (flag == 1) {
                      if (t1 == "1") {
                        t1 += "1";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t1;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t2 == "0") {
                        t2 += "1";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t2;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t3 == "1") {
                        t3 += "0";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t3;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t4 == "0") {
                        t4 += "0";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t4;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                    }
                    if (flag == 2) {
                      if (t1 == "11") {
                        t1 = "1";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t1;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t2 == "01") {
                        t2 = "1";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t2;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t3 == "10") {
                        t3 = "0";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t3;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t4 == "00") {
                        t4 = "0";
                        std::string result_temporary(Intermediate_temp[j]);
                        result_temporary += t4;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                    }
                  } else {
                    if (flag == 1) {
                      if (tt[0] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '1') {
                          std::string result_temporary(Intermediate_temp[j]);
                          result_temporary += t1;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[1] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '1') {
                          std::string result_temporary(Intermediate_temp[j]);
                          result_temporary += t2;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[2] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '0') {
                          std::string result_temporary(Intermediate_temp[j]);
                          result_temporary += t3;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[3] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '0') {
                          std::string result_temporary(Intermediate_temp[j]);
                          result_temporary += t4;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                    }
                    if (flag == 2) {
                      int count_sat1 = 0;
                      if (t1 == "11") {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          count_sat1 += 1;
                      }
                      if (t2 == "01") {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          count_sat1 += 1;
                      }
                      if (t3 == "10") {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          count_sat1 += 1;
                      }
                      if (t4 == "00") {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          count_sat1 += 1;
                      }
                      if (count_sat1 > 0)
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                  }
                }
                if (count_Gate_f == 1) {
                  Gate_judge[Gate_f - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_f);
                }
                if (count_Gate_b == 1) {
                  Gate_judge[Gate_b - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_b);
                }
              } else if (flag_cdccl == 1) {
                int flag_1 = 0;
                for (int j = 0; j < Intermediate_temp.size(); j++) {
                  if (Gate_judge[Gate_b - 1] < 0) {
                    flag_1 = 1;
                    if (tt[0] == target) {
                      if (Intermediate_temp_temp_F == "1") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[1] == target) {
                      if (Intermediate_temp_temp_F == "0") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[2] == target) {
                      if (Intermediate_temp_temp_F == "1") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[3] == target) {
                      if (Intermediate_temp_temp_F == "0") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                  } else {
                    if (tt[0] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                           '1') &&
                          (Intermediate_temp_temp_F == "1"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[1] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                           '1') &&
                          (Intermediate_temp_temp_F == "0"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[2] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                           '0') &&
                          (Intermediate_temp_temp_F == "1"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[3] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                           '0') &&
                          (Intermediate_temp_temp_F == "0"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                  }
                }
                if (flag_1 == 1) {
                  Gate_judge[Gate_b - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_b);
                }
              } else if (flag_cdccl == 2) {
                int flag_2 = 0;
                for (int j = 0; j < Intermediate_temp.size(); j++) {
                  if (Gate_judge[Gate_f - 1] < 0) {
                    flag_2 = 1;
                    if (tt[0] == target) {
                      if (Intermediate_temp_temp_B == "1") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[1] == target) {
                      if (Intermediate_temp_temp_B == "1") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[2] == target) {
                      if (Intermediate_temp_temp_B == "0") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[3] == target) {
                      if (Intermediate_temp_temp_B == "0") {
                        std::string Intermediate_temp_temp1(
                            Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                  } else {
                    if (tt[0] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] ==
                           '1') &&
                          (Intermediate_temp_temp_B == "1"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[1] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] ==
                           '0') &&
                          (Intermediate_temp_temp_B == "1"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[2] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] ==
                           '1') &&
                          (Intermediate_temp_temp_B == "0"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                    if (tt[3] == target) {
                      if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] ==
                           '0') &&
                          (Intermediate_temp_temp_B == "0"))
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                    }
                  }
                }
                if (flag_2 == 1) {
                  Gate_judge[Gate_f - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_f);
                }
              } else {
                for (int j = 0; j < Intermediate_temp.size(); j++) {
                  if (tt[0] == target) {
                    if ((Intermediate_temp_temp_F == "1") &&
                        (Intermediate_temp_temp_B == "1"))
                      Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                  }
                  if (tt[1] == target) {
                    if ((Intermediate_temp_temp_F == "0") &&
                        (Intermediate_temp_temp_B == "1"))
                      Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                  }
                  if (tt[2] == target) {
                    if ((Intermediate_temp_temp_F == "1") &&
                        (Intermediate_temp_temp_B == "0"))
                      Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                  }
                  if (tt[3] == target) {
                    if ((Intermediate_temp_temp_F == "0") &&
                        (Intermediate_temp_temp_B == "0"))
                      Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                  }
                }
              }
            }
            Intermediate_temp.assign(Intermediate_temp_temp.begin(),
                                     Intermediate_temp_temp.end());
            if (Intermediate_temp_temp.size() == 0) break;
          }
          temp1.Intermediate.assign(Intermediate_temp.begin(),
                                    Intermediate_temp.end());
          temp1.Gate.assign(Gate_temp.begin(), Gate_temp.end());
          for (int l = 0; l < temp1.Gate.size(); l++)
            temp1.Level[temp1.Gate[l] - 1] = level_current;

          int count = 0;  // whether there is a PI assignment
          for (int k = 0; k < temp1.Intermediate.size();
               k++)  // mix the Result and the Intermediate information in one
                     // level
          {
            count = 1;
            cdccl_impl_dag temp2;
            temp2.Level = temp1.Level;
            std::string Result_temp(temp1.Result);
            temp2.Gate.assign(temp1.Gate.begin(), temp1.Gate.end());
            std::string Intermediate_temp1(temp1.Intermediate[k]);
            int count1 = 0, count2 = 0;  // whether the assignment made
            for (int k11 = 0; k11 < temp1.Gate.size(); k11++) {
              if (temp1.Gate[k11] <
                  length3)  // if the Gate is smaller than length3, it is PI
              {
                temp2.Level[temp1.Gate[k11] - 1].Ordinate = ordinate;
                if ((temp1.Result[temp1.Gate[k11] - 1] == '2') ||
                    (temp1.Result[temp1.Gate[k11] - 1] ==
                     temp1.Intermediate[k][k11]))  // whether the PI can be
                                                   // assigned a value
                  Result_temp[temp1.Gate[k11] - 1] = temp1.Intermediate[k][k11];
                else
                  count1 = 1;  // if one assignment can't make, the count1 = 1
                Intermediate_temp1.erase(Intermediate_temp1.begin() +
                                         (k11 - count2));
                temp2.Gate.erase(temp2.Gate.begin() + (k11 - count2));
                count++, count2++;
              }
            }
            if (count1 == 0) {
              temp2.Result = Result_temp;
              temp2.Intermediate.push_back(Intermediate_temp1);
              for (int k12 = 0; k12 < temp2.Gate.size(); k12++)
                temp2.Level[temp2.Gate[k12] - 1].Ordinate = ordinate;
            }
            if (count == 1)
              break;
            else if (temp2.Result.size() > 0) {
              list_temp1.push_back(temp2);
              ordinate += 1;
              if (temp2.Gate.empty()) flag += 1;
            }
          }
          if (count == 1) {
            list_temp1.push_back(temp1);
            ordinate += 1;
            if (temp1.Gate.empty()) flag += 1;
          }
          temp1.Intermediate.clear();
        }
      }
      list.push_back(list_temp1);          // next level's information
      if (flag == list[level + 1].size())  // in one level, if all node's Gate
                                           // is empty, then break the loop
        break;
    }

    in.clear();
    for (int j = 0; j < list[list.size() - 1].size(); j++)  // all result
      in.push_back(list[list.size() - 1][j].Result);
  }

  vector<vector<std::string>> bench_expansion(vector<std::string> in) {
    vector<vector<std::string>> in_expansion;
    in_expansion.push_back(in);
    std::string in_end = in[in.size() - 2];
    for (int i = 0; i < in_end.size(); i++) {
      if (in_end[i] == '2') {
        vector<vector<std::string>> in_expansion_temp;
        for (int j = 0; j < in_expansion.size(); j++) {
          vector<std::string> in_temp_1(in_expansion[j]);
          vector<std::string> in_temp_0(in_expansion[j]);
          in_temp_1[in_expansion[j].size() - 2][i] = '1';
          in_temp_0[in_expansion[j].size() - 2][i] = '0';
          in_expansion_temp.push_back(in_temp_1);
          in_expansion_temp.push_back(in_temp_0);
        }
        in_expansion.assign(in_expansion_temp.begin(), in_expansion_temp.end());
      }
    }
    return in_expansion;
  }

 private:
  std::string& tt;
  int& input;
  exact_lut_params& ps;
  double min_area = 0.00;
  double min_delay = 0.00;
  int mapping_gate = 0;
};

void exact_lut_dag(std::string& tt, int& input, exact_lut_params& ps) {
  dag_impl p(tt, input, ps);
  p.run();
}

void exact_lut_dag_enu(std::string& tt, int& input, exact_lut_params& ps) {
  dag_impl p(tt, input, ps);
  p.run_enu();
}

void exact_lut_dag_enu_map(std::string& tt, int& input, exact_lut_params& ps) {
  dag_impl p(tt, input, ps);
  p.run_enu_map();
}

}  // namespace phyLS

#endif