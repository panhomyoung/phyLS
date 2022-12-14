/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file stpsat.hpp
 *
 * @brief Semi-Tensor Product based SAT
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#ifndef STPSAT_HPP
#define STPSAT_HPP

#include "../core/stp_sat.hpp"
#include <alice/alice.hpp>
#include <mockturtle/mockturtle.hpp>

using namespace std;

namespace alice
{
  class sat_command : public command
  {
  public:
    explicit sat_command(const environment::ptr &env) : command(env, " circuit-based SAT solver")
    {
      add_option("filename, -f", filename, "the input file name (CNF or BENCH)");
      add_option("single_po, -s", strategy, "select PO to solve (only in BENCH file)");
      add_flag("--verbose, -v", "verbose output");
    }

  protected:
    void execute()
    {
      in.clear();
      mtx.clear();
      expre.clear();
      tt.clear();
      vec.clear();

      string tmp;
      string s_cnf = ".cnf";
      string s_bench = ".bench";
      if (filename.find(s_bench) != string::npos)
        flag = 1;
      else if (filename.find(s_cnf) != string::npos)
        flag = 2;
      if (flag == 1)
      {
        ifstream fin_bench(filename);

        if (fin_bench.is_open())
        {
          while (getline(fin_bench, tmp))
            in.push_back(tmp);
          fin_bench.close();
          po = strategy;
          vector<string> &it = in;
          vector<vector<int>> &mtxvec = mtx;
          int &po_tmp = po;
          stopwatch<>::duration time{0};
          if (po < 0)
          {
            call_with_stopwatch(time, [&]()
                                { phyLS::cdccl_for_all(it, mtxvec); });
            if (it.size() == 0)
              cout << "UNSAT" << endl;
            else
              cout << "SAT" << endl;
            if (is_set("verbose"))
            {
              int count = 0;
              cout << "SAT Result : " << endl;
              for (string i : it)
              {
                cout << i << " ";
                count += 1;
                if (count == 10)
                {
                  cout << endl;
                  count = 0;
                }
              }
              cout << endl
                   << "Numbers of SAT Result : " << it.size() << endl;
            }
          }
          else
          {
            call_with_stopwatch(time, [&]()
                                { phyLS::cdccl_for_single_po(it, mtxvec, po_tmp); });
            if (it.size() == 0)
              cout << "UNSAT" << endl;
            else
              cout << "SAT" << endl;
            if (is_set("verbose"))
            {
              int count = 0;
              cout << " SAT Result of "
                   << "PO" << po << " : " << endl;
              for (string i : it)
              {
                cout << i << " ";
                count += 1;
                if (count == 10)
                {
                  cout << endl;
                  count = 0;
                }
              }
              cout << endl
                   << "Numbers of SAT Result : " << it.size() << endl;
            }
          }
          cout << fmt::format("[CPU time]: {:5.4f} seconds\n", to_seconds(time));
        }
        else
        {
          cerr << "Cannot open input file" << endl;
        }
      }
      else if (flag == 2)
      {
        ifstream fin_cnf(filename);

        stringstream buffer;
        buffer << fin_cnf.rdbuf();
        string str(buffer.str());
        string expression;
        vector<int> expre;
        for (int i = 6; i < (str.size() - 1); i++)
        {
          expression += str[i];
          if ((str[i] == ' ') || (str[i] == '\n'))
          {
            expression.pop_back();
            int intstr = atoi(expression.c_str());
            expre.push_back(intstr);
            expression.clear();
          }
        }

        fin_cnf.close();

        vector<int> &exp = expre;
        vector<string> &t = tt;
        vector<MatrixXi> &mtxvec = vec;
        stopwatch<>::duration time{0};

        call_with_stopwatch(time, [&]()
                            { phyLS::stp_cnf(exp, t, mtxvec); });

        if (t.size() == 0)
          cout << "UNSAT" << endl;
        else
          cout << "SAT" << endl;
        if (is_set("verbose"))
        {
          int count = 0;
          cout << "SAT Result : " << endl;
          for (string i : t)
          {
            cout << i << " ";
            count += 1;
            if (count == 10)
            {
              cout << endl;
              count = 0;
            }
          }
          cout << endl
               << "Numbers of SAT Result : " << t.size() << endl;
        }

        cout << fmt::format("[CPU time]: {:5.4f} seconds\n", to_seconds(time));
      }
    }

  private:
    string filename;
    string cnf;
    int strategy = -1;
    int po;
    int flag = 0;

    vector<string> in;
    vector<vector<int>> mtx;

    vector<int> expre;
    vector<string> tt;
    vector<MatrixXi> vec;
  };

  ALICE_ADD_COMMAND(sat, "Verification")
} // namespace alice

#endif
