/* phyLS: powerful heightened yielded Logic Synthesis
 * Copyright (C) 2022 */

/**
 * @file stp_sat.hpp
 *
 * @brief Semi-Tensor Product based SAT
 *
 * @author Homyoung
 * @since  2022/12/14
 */

#pragma once

#include "matrix.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace phyLS
{
  struct coordinate
  {
    int Abscissa;
    int Ordinate;
    int parameter_Intermediate;
    int parameter_Gate;
  };

  struct cdccl_impl
  {
    vector<string> Intermediate;
    string Result;
    vector<int> Gate;
    vector<coordinate> Level; // Define the network as a coordinate system
  };

  class stp_cdccl_impl
  {
  public:
    stp_cdccl_impl(vector<string> &in, vector<vector<int>> &mtxvec)
        : in(in), mtxvec(mtxvec)
    {
    }

    void run_normal()
    {
      parser_from_bench(in, mtxvec);
      bench_solve(in);
    }

  private:
    void parser_from_bench(vector<string> &in, vector<vector<int>> &mtxvec)
    {
      vector<string> tt;
      string s1 = "INPUT";
      string s2 = "OUTPUT";
      string s3 = "LUT";
      string s4 = ",";
      int count1 = 0, count2 = 0;
      for (int i = 0; i < in.size(); i++)
      {
        if (in[i].find(s1) != string::npos)
        {
          count1 += 1;
        }
        else if (in[i].find(s2) != string::npos)
        {
          count2 += 1;
        }
        else if (in[i].find(s3) != string::npos)
        {
          if (in[i].find(s4) != string::npos)
          {
            vector<int> temp;
            string flag0 = "=";
            string flag1 = "x";
            string flag2 = "(";
            string flag3 = ",";
            string flag4 = ")";
            int p0 = in[i].find(flag0);
            int p1 = in[i].find(flag1);
            int p2 = in[i].find(flag2);
            int p3 = in[i].find(flag3);
            int p4 = in[i].find(flag4);
            string temp0 = in[i].substr(1, p0 - 2);
            string temp1 = in[i].substr(p1 + 1, p2 - p1 - 2);
            string temp2 = in[i].substr(p2 + 2, p3 - p2 - 2);
            string temp3 = in[i].substr(p3 + 3, p4 - p3 - 3);
            int intstr0 = atoi(temp0.c_str());
            int intstr1 = atoi(temp2.c_str());
            int intstr2 = atoi(temp3.c_str());
            temp.push_back(intstr1);
            temp.push_back(intstr2);
            temp.push_back(intstr0);
            string temp4;
            int len = temp1.length();
            for (int i = 0; i < len; i++)
            {
              switch (temp1[i])
              {
              case '0':
                temp4.append("0000");
                continue;
              case '1':
                temp4.append("0001");
                continue;
              case '2':
                temp4.append("0010");
                continue;
              case '3':
                temp4.append("0011");
                continue;
              case '4':
                temp4.append("0100");
                continue;
              case '5':
                temp4.append("0101");
                continue;
              case '6':
                temp4.append("0110");
                continue;
              case '7':
                temp4.append("0111");
                continue;
              case '8':
                temp4.append("1000");
                continue;
              case '9':
                temp4.append("1001");
                continue;
              case 'a':
                temp4.append("1010");
                continue;
              case 'b':
                temp4.append("1011");
                continue;
              case 'c':
                temp4.append("1100");
                continue;
              case 'd':
                temp4.append("1101");
                continue;
              case 'e':
                temp4.append("1110");
                continue;
              case 'f':
                temp4.append("1111");
                continue;
              }
            }
            tt.push_back(temp4);
            mtxvec.push_back(temp);
          }
          else
          {
            vector<int> temp;
            string flag1 = "x";
            string flag2 = "(";
            string flag3 = ")";
            int p1 = in[i].find(flag1);
            int p2 = in[i].find(flag2);
            int p3 = in[i].find(flag3);
            string temp1 = in[i].substr(p1 + 1, p2 - p1 - 1);
            string temp2 = in[i].substr(p2 + 2, p3 - p2 - 2);
            int intstr1 = atoi(temp2.c_str());
            temp.push_back(intstr1);
            string temp3;
            int len = temp1.length();
            for (int i = 0; i < len; i++)
            {
              switch (temp1[i])
              {
              case '0':
                temp3.append("00");
                break;
              case '1':
                temp3.append("01");
                break;
              case '2':
                temp3.append("10");
                break;
              }
            }
            tt.push_back(temp3);
            mtxvec.push_back(temp);
          }
        }
      }
      vector<int> temp;
      temp.push_back(count1);
      temp.push_back(count2);
      mtxvec.push_back(temp);
      in.assign(tt.begin(), tt.end());
    }

    void bench_solve(vector<string> &in)
    {
      int length1 = mtxvec[mtxvec.size() - 1][0];           // number of primary input
      int length2 = mtxvec[mtxvec.size() - 1][1];           // number of primary output
      int length3 = mtxvec[0][2];                           // the minimum minuend
      int length4 = mtxvec[mtxvec.size() - 2 - length2][2]; // the maximum variable
      vector<vector<cdccl_impl>> list;                      // the solution space is two dimension vector
      vector<cdccl_impl> list_temp;
      cdccl_impl list_temp_temp;

      string Result_temp(length1, '2'); // temporary result
      coordinate Level_temp;
      Level_temp.Abscissa = -1;
      Level_temp.Ordinate = -1;
      Level_temp.parameter_Intermediate = -1;
      Level_temp.parameter_Gate = -1;
      list_temp_temp.Level.resize(length4, Level_temp); // Initialize level as a space with the size of variables(length4)
      /*
       * initialization
       */
      coordinate Gate_level;
      Gate_level.Abscissa = 0;
      Gate_level.Ordinate = 0;
      Gate_level.parameter_Intermediate = -1;
      Gate_level.parameter_Gate = -1;
      string Intermediate_temp;
      for (int i = mtxvec.size() - 1 - length2; i < mtxvec.size() - 1; i++) // the original intermediate
      {
        if (mtxvec[i][0] < length3)
        {
          if (in[i][0] == '1')
          {
            Result_temp[mtxvec[i][0] - 1] = '1';
          }
          else
          {
            Result_temp[mtxvec[i][0] - 1] = '0';
          }
        }
        else
        {
          list_temp_temp.Gate.push_back(mtxvec[i][0]);
          if (in[i][0] == '1')
          {
            Intermediate_temp += "1";
          }
          else
          {
            Intermediate_temp += "0";
          }
        }
        list_temp_temp.Level[mtxvec[i][0] - 1] = Gate_level;
      }
      list_temp_temp.Result = Result_temp;
      list_temp_temp.Intermediate.push_back(Intermediate_temp);
      list_temp.push_back(list_temp_temp); // level 0
      list.push_back(list_temp);
      /*
       * The first level information
       */
      int count1 = 0;
      for (int level = 0;; level++) // the computation process
      {
        int flag = 0, ordinate = 0; // the flag of the end of the loop & the ordinate of each level's gate
        vector<cdccl_impl> list_temp1;
        for (int k = 0; k < list[level].size(); k++)
        {
          cdccl_impl temp1;                     // temporary gate
          temp1.Result = list[level][k].Result; // first, next level's Result is same as the front level
          for (int j = 0; j < list[level][k].Intermediate.size(); j++)
          {
            temp1.Level = list[level][k].Level; // next level's Level information
            for (int j1 = 0; j1 < list[level][k].Gate.size(); j1++)
            {
              temp1.Level[list[level][k].Gate[j1] - 1].parameter_Intermediate = j;
              temp1.Level[list[level][k].Gate[j1] - 1].parameter_Gate = j1;
            }

            coordinate level_current; // new level
            level_current.Abscissa = level + 1;
            level_current.parameter_Intermediate = -1;
            level_current.parameter_Gate = -1;
            level_current.Ordinate = ordinate;

            temp1.Gate.assign(list[level][k].Gate.begin(), list[level][k].Gate.end()); // need more!!!!!
            temp1.Intermediate.push_back(list[level][k].Intermediate[j]);

            vector<string> Intermediate_temp;
            vector<int> Gate_temp;
            vector<int> Gate_judge(length4, -1);
            int count_cdccl = 0;
            for (int i = 0; i < temp1.Gate.size(); i++)
            {
              int length = temp1.Gate[i] - length3;
              int Gate_f = mtxvec[length][0];         // the front Gate variable
              int Gate_b = mtxvec[length][1];         // the behind Gate variable
              string tt = in[length];                 // the correndsponding Truth Table
              char target = temp1.Intermediate[0][i]; // the SAT target
              vector<string> Intermediate_temp_temp;

              int flag_cdccl = 0;
              string Intermediate_temp_temp_F, Intermediate_temp_temp_B;
              if (temp1.Level[Gate_f - 1].Abscissa >= 0)
              {
                if (Gate_f < length3)
                {
                  char contrast_R1 = list[temp1.Level[Gate_f - 1].Abscissa][temp1.Level[Gate_f - 1].Ordinate].Result[Gate_f - 1];
                  Intermediate_temp_temp_F.push_back(contrast_R1);
                }
                else
                {
                  char contrast_I1 = list[temp1.Level[Gate_f - 1].Abscissa][temp1.Level[Gate_f - 1].Ordinate].Intermediate[temp1.Level[Gate_f - 1].parameter_Intermediate][temp1.Level[Gate_f - 1].parameter_Gate];
                  Intermediate_temp_temp_F.push_back(contrast_I1);
                }
                flag_cdccl += 1;
              }
              if (temp1.Level[Gate_b - 1].Abscissa >= 0)
              {
                if (Gate_b < length3)
                {
                  char contrast_R2 = list[temp1.Level[Gate_b - 1].Abscissa][temp1.Level[Gate_b - 1].Ordinate].Result[Gate_b - 1];
                  Intermediate_temp_temp_B.push_back(contrast_R2);
                }
                else
                {
                  char contrast_I2 = list[temp1.Level[Gate_b - 1].Abscissa][temp1.Level[Gate_b - 1].Ordinate].Intermediate[temp1.Level[Gate_b - 1].parameter_Intermediate][temp1.Level[Gate_b - 1].parameter_Gate];
                  Intermediate_temp_temp_B.push_back(contrast_I2);
                }
                flag_cdccl += 2;
              }
              if (Intermediate_temp.size() == 0)
              {
                if (flag_cdccl == 0)
                {
                  Gate_judge[Gate_f - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_judge[Gate_b - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_f);
                  Gate_temp.push_back(Gate_b);
                  if (tt[0] == target)
                  {
                    Intermediate_temp_temp.push_back("11");
                  }
                  if (tt[1] == target)
                  {
                    Intermediate_temp_temp.push_back("01");
                  }
                  if (tt[2] == target)
                  {
                    Intermediate_temp_temp.push_back("10");
                  }
                  if (tt[3] == target)
                  {
                    Intermediate_temp_temp.push_back("00");
                  }
                }
                else if (flag_cdccl == 1)
                {
                  Gate_judge[Gate_b - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_b);
                  if (tt[0] == target)
                  {
                    if (Intermediate_temp_temp_F == "1")
                    {
                      Intermediate_temp_temp.push_back("1");
                    }
                  }
                  if (tt[1] == target)
                  {
                    if (Intermediate_temp_temp_F == "0")
                    {
                      Intermediate_temp_temp.push_back("1");
                    }
                  }
                  if (tt[2] == target)
                  {
                    if (Intermediate_temp_temp_F == "1")
                    {
                      Intermediate_temp_temp.push_back("0");
                    }
                  }
                  if (tt[3] == target)
                  {
                    if (Intermediate_temp_temp_F == "0")
                    {
                      Intermediate_temp_temp.push_back("0");
                    }
                  }
                }
                else if (flag_cdccl == 2)
                {
                  Gate_judge[Gate_f - 1] = count_cdccl;
                  count_cdccl += 1;
                  Gate_temp.push_back(Gate_f);
                  if (tt[0] == target)
                  {
                    if (Intermediate_temp_temp_B == "1")
                    {
                      Intermediate_temp_temp.push_back("1");
                    }
                  }
                  if (tt[1] == target)
                  {
                    if (Intermediate_temp_temp_B == "1")
                    {
                      Intermediate_temp_temp.push_back("0");
                    }
                  }
                  if (tt[2] == target)
                  {
                    if (Intermediate_temp_temp_B == "0")
                    {
                      Intermediate_temp_temp.push_back("1");
                    }
                  }
                  if (tt[3] == target)
                  {
                    if (Intermediate_temp_temp_B == "0")
                    {
                      Intermediate_temp_temp.push_back("0");
                    }
                  }
                }
                else
                {
                  int t0 = 0, t1 = 0, t2 = 0, t3 = 0;
                  if (tt[0] == target)
                  {
                    if ((Intermediate_temp_temp_F == "1") && (Intermediate_temp_temp_B == "1"))
                    {
                      t0 = 1;
                    }
                  }
                  if (tt[1] == target)
                  {
                    if ((Intermediate_temp_temp_F == "0") && (Intermediate_temp_temp_B == "1"))
                    {
                      t1 = 1;
                    }
                  }
                  if (tt[2] == target)
                  {
                    if ((Intermediate_temp_temp_F == "1") && (Intermediate_temp_temp_B == "0"))
                    {
                      t2 = 1;
                    }
                  }
                  if (tt[3] == target)
                  {
                    if ((Intermediate_temp_temp_F == "0") && (Intermediate_temp_temp_B == "0"))
                    {
                      t3 = 1;
                    }
                  }
                  if ((t0 == 1) || (t1 == 1) || (t2 == 1) || (t3 == 1))
                  {
                    Gate_judge[Gate_f - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_judge[Gate_b - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_temp.push_back(Gate_f);
                    Gate_temp.push_back(Gate_b);
                    if (t0 == 1)
                    {
                      Intermediate_temp_temp.push_back("11");
                    }
                    if (t1 == 1)
                    {
                      Intermediate_temp_temp.push_back("01");
                    }
                    if (t2 == 1)
                    {
                      Intermediate_temp_temp.push_back("10");
                    }
                    if (t3 == 1)
                    {
                      Intermediate_temp_temp.push_back("00");
                    }
                  }
                }
              }
              else
              {
                if (flag_cdccl == 0)
                {
                  int count_Gate_f = 0, count_Gate_b = 0;
                  for (int j = 0; j < Intermediate_temp.size(); j++)
                  {
                    int flag;
                    string t1, t2, t3, t4;
                    if (Gate_judge[Gate_f - 1] < 0)
                    {
                      count_Gate_f = 1;
                      if (tt[0] == target)
                      {
                        t1 = "1";
                      }
                      if (tt[1] == target)
                      {
                        t2 = "0";
                      }
                      if (tt[2] == target)
                      {
                        t3 = "1";
                      }
                      if (tt[3] == target)
                      {
                        t4 = "0";
                      }
                      flag = 1;
                    }
                    else
                    {
                      int count_sat = 0;
                      if (tt[0] == target)
                      {
                        if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1')
                        {
                          t1 = "11";
                          count_sat += 1;
                        }
                      }
                      if (tt[1] == target)
                      {
                        if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0')
                        {
                          t2 = "01";
                          count_sat += 1;
                        }
                      }
                      if (tt[2] == target)
                      {
                        if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1')
                        {
                          t3 = "10";
                          count_sat += 1;
                        }
                      }
                      if (tt[3] == target)
                      {
                        if (Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0')
                        {
                          t4 = "00";
                          count_sat += 1;
                        }
                      }
                      if (count_sat == 0)
                      {
                        continue;
                      }
                      flag = 2;
                    }
                    if (Gate_judge[Gate_b - 1] < 0)
                    {
                      count_Gate_b = 1;
                      if (flag == 1)
                      {
                        if (t1 == "1")
                        {
                          t1 += "1";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t1;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t2 == "0")
                        {
                          t2 += "1";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t2;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t3 == "1")
                        {
                          t3 += "0";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t3;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t4 == "0")
                        {
                          t4 += "0";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t4;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (flag == 2)
                      {
                        if (t1 == "11")
                        {
                          t1 = "1";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t1;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t2 == "01")
                        {
                          t2 = "1";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t2;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t3 == "10")
                        {
                          t3 = "0";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t3;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                        if (t4 == "00")
                        {
                          t4 = "0";
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t4;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                    }
                    else
                    {
                      if (flag == 1)
                      {
                        if (tt[0] == target)
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          {
                            string result_temporary(Intermediate_temp[j]);
                            result_temporary += t1;
                            Intermediate_temp_temp.push_back(result_temporary);
                          }
                        }
                        if (tt[1] == target)
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          {
                            string result_temporary(Intermediate_temp[j]);
                            result_temporary += t2;
                            Intermediate_temp_temp.push_back(result_temporary);
                          }
                        }
                        if (tt[2] == target)
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          {
                            string result_temporary(Intermediate_temp[j]);
                            result_temporary += t3;
                            Intermediate_temp_temp.push_back(result_temporary);
                          }
                        }
                        if (tt[3] == target)
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          {
                            string result_temporary(Intermediate_temp[j]);
                            result_temporary += t4;
                            Intermediate_temp_temp.push_back(result_temporary);
                          }
                        }
                      }
                      if (flag == 2)
                      {
                        int count_sat1 = 0;
                        if (t1 == "11")
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          {
                            count_sat1 += 1;
                          }
                        }
                        if (t2 == "01")
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1')
                          {
                            count_sat1 += 1;
                          }
                        }
                        if (t3 == "10")
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          {
                            count_sat1 += 1;
                          }
                        }
                        if (t4 == "00")
                        {
                          if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0')
                          {
                            count_sat1 += 1;
                          }
                        }
                        if (count_sat1 > 0)
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                    }
                  }
                  if (count_Gate_f == 1)
                  {
                    Gate_judge[Gate_f - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_temp.push_back(Gate_f);
                  }
                  if (count_Gate_b == 1)
                  {
                    Gate_judge[Gate_b - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_temp.push_back(Gate_b);
                  }
                }
                else if (flag_cdccl == 1)
                {
                  int flag_1 = 0;
                  for (int j = 0; j < Intermediate_temp.size(); j++)
                  {
                    if (Gate_judge[Gate_b - 1] < 0)
                    {
                      flag_1 = 1;
                      if (tt[0] == target)
                      {
                        if (Intermediate_temp_temp_F == "1")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "1";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[1] == target)
                      {
                        if (Intermediate_temp_temp_F == "0")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "1";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[2] == target)
                      {
                        if (Intermediate_temp_temp_F == "1")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "0";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[3] == target)
                      {
                        if (Intermediate_temp_temp_F == "0")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "0";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                    }
                    else
                    {
                      if (tt[0] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1') && (Intermediate_temp_temp_F == "1"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[1] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '1') && (Intermediate_temp_temp_F == "0"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[2] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0') && (Intermediate_temp_temp_F == "1"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[3] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_b - 1]] == '0') && (Intermediate_temp_temp_F == "0"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                    }
                  }
                  if (flag_1 == 1)
                  {
                    Gate_judge[Gate_b - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_temp.push_back(Gate_b);
                  }
                }
                else if (flag_cdccl == 2)
                {
                  int flag_2 = 0;
                  for (int j = 0; j < Intermediate_temp.size(); j++)
                  {
                    if (Gate_judge[Gate_f - 1] < 0)
                    {
                      flag_2 = 1;
                      if (tt[0] == target)
                      {
                        if (Intermediate_temp_temp_B == "1")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "1";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[1] == target)
                      {
                        if (Intermediate_temp_temp_B == "1")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "0";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[2] == target)
                      {
                        if (Intermediate_temp_temp_B == "0")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "1";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                      if (tt[3] == target)
                      {
                        if (Intermediate_temp_temp_B == "0")
                        {
                          string Intermediate_temp_temp1(Intermediate_temp[j]);
                          Intermediate_temp_temp1 += "0";
                          Intermediate_temp_temp.push_back(Intermediate_temp_temp1);
                        }
                      }
                    }
                    else
                    {
                      if (tt[0] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1') && (Intermediate_temp_temp_B == "1"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[1] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0') && (Intermediate_temp_temp_B == "1"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[2] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '1') && (Intermediate_temp_temp_B == "0"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                      if (tt[3] == target)
                      {
                        if ((Intermediate_temp[j][Gate_judge[Gate_f - 1]] == '0') && (Intermediate_temp_temp_B == "0"))
                        {
                          Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                        }
                      }
                    }
                  }
                  if (flag_2 == 1)
                  {
                    Gate_judge[Gate_f - 1] = count_cdccl;
                    count_cdccl += 1;
                    Gate_temp.push_back(Gate_f);
                  }
                }
                else
                {
                  for (int j = 0; j < Intermediate_temp.size(); j++)
                  {
                    if (tt[0] == target)
                    {
                      if ((Intermediate_temp_temp_F == "1") && (Intermediate_temp_temp_B == "1"))
                      {
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                      }
                    }
                    if (tt[1] == target)
                    {
                      if ((Intermediate_temp_temp_F == "0") && (Intermediate_temp_temp_B == "1"))
                      {
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                      }
                    }
                    if (tt[2] == target)
                    {
                      if ((Intermediate_temp_temp_F == "1") && (Intermediate_temp_temp_B == "0"))
                      {
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                      }
                    }
                    if (tt[3] == target)
                    {
                      if ((Intermediate_temp_temp_F == "0") && (Intermediate_temp_temp_B == "0"))
                      {
                        Intermediate_temp_temp.push_back(Intermediate_temp[j]);
                      }
                    }
                  }
                }
              }
              Intermediate_temp.assign(Intermediate_temp_temp.begin(), Intermediate_temp_temp.end());
              if (Intermediate_temp_temp.size() == 0)
              {
                break;
              }
            }
            temp1.Intermediate.assign(Intermediate_temp.begin(), Intermediate_temp.end());
            temp1.Gate.assign(Gate_temp.begin(), Gate_temp.end());
            for (int l = 0; l < temp1.Gate.size(); l++)
            {
              temp1.Level[temp1.Gate[l] - 1] = level_current;
            }

            int count = 0;                                      // whether there is a PI assignment
            for (int k = 0; k < temp1.Intermediate.size(); k++) // mix the Result and the Intermediate information in one level
            {
              count = 1;
              cdccl_impl temp2;
              temp2.Level = temp1.Level;
              string Result_temp(temp1.Result);
              temp2.Gate.assign(temp1.Gate.begin(), temp1.Gate.end());
              string Intermediate_temp1(temp1.Intermediate[k]);
              int count1 = 0, count2 = 0; // whether the assignment made
              for (int k11 = 0; k11 < temp1.Gate.size(); k11++)
              {
                if (temp1.Gate[k11] < length3) // if the Gate is smaller than length3, it is PI
                {
                  temp2.Level[temp1.Gate[k11] - 1].Ordinate = ordinate;
                  if ((temp1.Result[temp1.Gate[k11] - 1] == '2') || (temp1.Result[temp1.Gate[k11] - 1] == temp1.Intermediate[k][k11])) // whether the PI can be assigned a value
                  {
                    Result_temp[temp1.Gate[k11] - 1] = temp1.Intermediate[k][k11];
                  }
                  else
                  {
                    count1 = 1; // if one assignment can't make, the count1 = 1
                  }
                  Intermediate_temp1.erase(Intermediate_temp1.begin() + (k11 - count2));
                  temp2.Gate.erase(temp2.Gate.begin() + (k11 - count2));
                  count++, count2++;
                }
              }
              if (count1 == 0)
              {
                temp2.Result = Result_temp;
                temp2.Intermediate.push_back(Intermediate_temp1);
                for (int k12 = 0; k12 < temp2.Gate.size(); k12++)
                {
                  temp2.Level[temp2.Gate[k12] - 1].Ordinate = ordinate;
                }
              }
              if (count == 1)
              {
                break;
              }
              else if (temp2.Result.size() > 0)
              {
                list_temp1.push_back(temp2);
                ordinate += 1;
                if (temp2.Gate.empty())
                {
                  flag += 1;
                }
              }
            }
            if (count == 1)
            {
              list_temp1.push_back(temp1);
              ordinate += 1;
              if (temp1.Gate.empty())
              {
                flag += 1;
              }
            }
            temp1.Intermediate.clear();
          }
        }
        list.push_back(list_temp1);         // next level's information
        if (flag == list[level + 1].size()) // in one level, if all node's Gate is empty, then break the loop
        {
          break;
        }
      }

      in.clear();
      for (int j = 0; j < list[list.size() - 1].size(); j++) // all result
      {
        in.push_back(list[list.size() - 1][j].Result);
      }
    }

  private:
    vector<string> &in;
    vector<vector<int>> &mtxvec;
  };

  class stp_cdccl_impl2
  {
  public:
    stp_cdccl_impl2(vector<string> &in, vector<vector<int>> &mtxvec, int &po)
        : in(in), mtxvec(mtxvec), po(po)
    {
    }

    void run_single_po()
    {
      parser_from_bench(in, mtxvec);
      bench_solve_single_po(in, mtxvec, po);
    }

  private:
    void parser_from_bench(vector<string> &in, vector<vector<int>> &mtxvec)
    {
      vector<string> tt;
      string s1 = "INPUT";
      string s2 = "OUTPUT";
      string s3 = "LUT";
      string s4 = ",";
      int count1 = 0, count2 = 0;
      for (int i = 0; i < in.size(); i++)
      {
        if (in[i].find(s1) != string::npos)
        {
          count1 += 1;
        }
        else if (in[i].find(s2) != string::npos)
        {
          count2 += 1;
        }
        else if (in[i].find(s3) != string::npos)
        {
          if (in[i].find(s4) != string::npos)
          {
            vector<int> temp;
            string flag0 = "=";
            string flag1 = "x";
            string flag2 = "(";
            string flag3 = ",";
            string flag4 = ")";
            int p0 = in[i].find(flag0);
            int p1 = in[i].find(flag1);
            int p2 = in[i].find(flag2);
            int p3 = in[i].find(flag3);
            int p4 = in[i].find(flag4);
            string temp0 = in[i].substr(1, p0 - 2);
            string temp1 = in[i].substr(p1 + 1, p2 - p1 - 2);
            string temp2 = in[i].substr(p2 + 2, p3 - p2 - 2);
            string temp3 = in[i].substr(p3 + 3, p4 - p3 - 3);
            int intstr0 = atoi(temp0.c_str());
            int intstr1 = atoi(temp2.c_str());
            int intstr2 = atoi(temp3.c_str());
            temp.push_back(intstr1);
            temp.push_back(intstr2);
            temp.push_back(intstr0);
            string temp4;
            int len = temp1.length();
            for (int i = 0; i < len; i++)
            {
              switch (temp1[i])
              {
              case '0':
                temp4.append("0000");
                continue;
              case '1':
                temp4.append("0001");
                continue;
              case '2':
                temp4.append("0010");
                continue;
              case '3':
                temp4.append("0011");
                continue;
              case '4':
                temp4.append("0100");
                continue;
              case '5':
                temp4.append("0101");
                continue;
              case '6':
                temp4.append("0110");
                continue;
              case '7':
                temp4.append("0111");
                continue;
              case '8':
                temp4.append("1000");
                continue;
              case '9':
                temp4.append("1001");
                continue;
              case 'a':
                temp4.append("1010");
                continue;
              case 'b':
                temp4.append("1011");
                continue;
              case 'c':
                temp4.append("1100");
                continue;
              case 'd':
                temp4.append("1101");
                continue;
              case 'e':
                temp4.append("1110");
                continue;
              case 'f':
                temp4.append("1111");
                continue;
              }
            }
            tt.push_back(temp4);
            mtxvec.push_back(temp);
          }
          else
          {
            vector<int> temp;
            string flag1 = "x";
            string flag2 = "(";
            string flag3 = ")";
            int p1 = in[i].find(flag1);
            int p2 = in[i].find(flag2);
            int p3 = in[i].find(flag3);
            string temp1 = in[i].substr(p1 + 1, p2 - p1 - 1);
            string temp2 = in[i].substr(p2 + 2, p3 - p2 - 2);
            int intstr1 = atoi(temp2.c_str());
            temp.push_back(intstr1);
            string temp3;
            int len = temp1.length();
            for (int i = 0; i < len; i++)
            {
              switch (temp1[i])
              {
              case '0':
                temp3.append("00");
                break;
              case '1':
                temp3.append("01");
                break;
              case '2':
                temp3.append("10");
                break;
              }
            }
            tt.push_back(temp3);
            mtxvec.push_back(temp);
          }
        }
      }
      vector<int> temp;
      temp.push_back(count1);
      temp.push_back(count2);
      mtxvec.push_back(temp);
      in.assign(tt.begin(), tt.end());
    }

    void bench_solve_single_po(vector<string> &in, vector<vector<int>> &mtxvec, int &po)
    {
      int length0 = mtxvec.size() - 1;
      int length1 = mtxvec[length0][1];
      int length2 = mtxvec[0][2];
      int length3 = mtxvec[length0][0];
      int po_tmp = mtxvec[length0 - length1 + po][0];
      vector<string> temp;
      string result_tmp1(length3, '2');
      vector<string> result_tmp;
      matrix_propagation(in[length0 - length1 + po], result_tmp, '1');
      if (po_tmp >= length2)
      {
        int y = po_tmp - length2;
        solve(mtxvec, in, result_tmp[0][0], y, result_tmp1, temp);
      }
      if (temp.size() == 0)
      {
        in.clear();
      }
      in.assign(temp.begin(), temp.end());
    }

    void solve(vector<vector<int>> &mtxvec, vector<string> &in, char target, int i, string &result1, vector<string> &temp)
    {
      int length1 = mtxvec[0][2];
      int length2 = mtxvec[mtxvec.size() - 1][0];
      int length3 = length1 - length2;
      vector<string> result_tmp;
      vector<string> result;
      string reset = result1;
      matrix_propagation(in[i], result_tmp, target);
      if ((mtxvec[i][0] < length1) && (mtxvec[i][1] < length1))
      {
        for (int j = 0; j < result_tmp.size(); j++)
        {
          int count = 0;
          result1 = reset;
          if ((result1[mtxvec[i][0] - length3] == '2') || (result1[mtxvec[i][0] - length3] == result_tmp[j][0]))
          {
            result1[mtxvec[i][0] - length3] = result_tmp[j][0];
            count += 1;
          }
          if ((result1[mtxvec[i][1] - length3] == '2') || (result1[mtxvec[i][1] - length3] == result_tmp[j][1]))
          {
            result1[mtxvec[i][1] - length3] = result_tmp[j][1];
            count += 1;
          }
          if (count == 2)
          {
            result.push_back(result1);
          }
        }
        temp.assign(result.begin(), result.end());
      }
      else if ((mtxvec[i][0] < length1) && (mtxvec[i][1] >= length1))
      {
        for (int j = 0; j < result_tmp.size(); j++)
        {
          int count = 0;
          result1 = reset;
          if ((result1[mtxvec[i][0] - length3] == '2') || (result1[mtxvec[i][0] - length3] == result_tmp[j][0]))
          {
            result1[mtxvec[i][0] - length3] = result_tmp[j][0];
            count += 1;
          }
          int i2 = mtxvec[i][1] - length1;
          if (count == 1)
          {
            solve(mtxvec, in, result_tmp[j][1], i2, result1, temp);
          }
          result.insert(result.end(), temp.begin(), temp.end());
          temp.clear();
        }
        temp.assign(result.begin(), result.end());
      }
      else if ((mtxvec[i][0] >= length1) && (mtxvec[i][1] >= length1))
      {
        for (int j = 0; j < result_tmp.size(); j++)
        {
          result1 = reset;
          int i1 = mtxvec[i][0] - length1;
          solve(mtxvec, in, result_tmp[j][0], i1, result1, temp);
          int i2 = mtxvec[i][1] - length1;
          vector<string> temp_temp(temp);
          for (int k = 0; k < temp_temp.size(); k++)
          {
            result1 = temp_temp[k];
            solve(mtxvec, in, result_tmp[j][1], i2, result1, temp);
            result.insert(result.end(), temp.begin(), temp.end());
            temp.clear();
          }
        }
        temp.assign(result.begin(), result.end());
      }
    }

    void matrix_propagation(string tt, vector<string> &result, char target)
    {
      int n = tt.size();
      if (n == 2)
      {
        if (tt[0] == target)
        {
          string result_tmp1 = "1";
          result.push_back(result_tmp1);
        }
        if (tt[1] == target)
        {
          string result_tmp2 = "0";
          result.push_back(result_tmp2);
        }
      }
      else if (n == 4)
      {
        if (tt[0] == target)
        {
          string result_tmp3 = "11";
          result.push_back(result_tmp3);
        }
        if (tt[1] == target)
        {
          string result_tmp4 = "01"; //因为真值表是反的，所以两个变量位置换一下
          result.push_back(result_tmp4);
        }
        if (tt[2] == target)
        {
          string result_tmp5 = "10";
          result.push_back(result_tmp5);
        }
        if (tt[3] == target)
        {
          string result_tmp6 = "00";
          result.push_back(result_tmp6);
        }
      }
    }

  private:
    vector<string> &in;
    vector<vector<int>> &mtxvec;
    int &po;
  };

  class stp_cnf_impl
  {
  public:
    stp_cnf_impl(vector<int> &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
        : expression(expression), tt(tt), mtxvec(mtxvec)
    {
    }

    void run_cnf()
    {
      parser_from_expression(tt, expression);
      matrix_mapping(tt, mtxvec);
      stp_cnf(tt, mtxvec, expression);
      stp_result(tt, expression);
    }

  private:
    void parser_from_expression(vector<string> &tt, vector<int> &expression)
    {
      string tmp0 = "MN";
      string tmp1 = "END";
      string tmp2 = "MP";
      string tmp3 = "MD";
      string tmp4 = "A";
      for (int i = 2; i < expression.size(); i++)
      {
        if (expression[i] == 0)
        {
          tt.push_back(tmp1);
        }
        else
        {
          if ((expression[i - 1] == 0) && (expression[i + 1] == 0))
          {
            if (expression[i] < 0)
            {
              tt.push_back(tmp0);
              tt.push_back(tmp4);
            }
            else
            {
              tt.push_back(tmp2);
              tt.push_back(tmp4);
            }
          }
          else if (expression[i + 1] == 0)
          {
            if (expression[i] < 0)
            {
              tt.push_back(tmp0);
              tt.push_back(tmp4);
            }
            else
            {
              tt.push_back(tmp4);
            }
          }
          else
          {
            tt.push_back(tmp3);
            if (expression[i] < 0)
            {
              tt.push_back(tmp0);
            }
            tt.push_back(tmp4);
          }
        }
        expression[i] = abs(expression[i]);
      }
    }

    void matrix_mapping(vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
      for (int ix = 0; ix < tt.size(); ix++)
      {
        if (tt[ix] == "MN")
        {
          MatrixXi mtxtmp1(2, 2);
          mtxtmp1(0, 0) = 0;
          mtxtmp1(0, 1) = 1;
          mtxtmp1(1, 0) = 1;
          mtxtmp1(1, 1) = 0;
          mtxvec.push_back(mtxtmp1);
        }
        else if (tt[ix] == "MP")
        {
          MatrixXi mtxtmp4(2, 2);
          mtxtmp4(0, 0) = 1;
          mtxtmp4(0, 1) = 0;
          mtxtmp4(1, 0) = 0;
          mtxtmp4(1, 1) = 1;
          mtxvec.push_back(mtxtmp4);
        }
        else if (tt[ix] == "MD")
        {
          MatrixXi mtxtmp2(2, 4);
          mtxtmp2(0, 0) = 1;
          mtxtmp2(0, 1) = 1;
          mtxtmp2(0, 2) = 1;
          mtxtmp2(0, 3) = 0;
          mtxtmp2(1, 0) = 0;
          mtxtmp2(1, 1) = 0;
          mtxtmp2(1, 2) = 0;
          mtxtmp2(1, 3) = 1;
          mtxvec.push_back(mtxtmp2);
        }
        else
        {
          MatrixXi mtxtmp3(2, 1);
          mtxtmp3(0, 0) = 1;
          mtxtmp3(1, 0) = 0;
          mtxvec.push_back(mtxtmp3);
        }
      }
    }

    void stp_cnf(vector<string> &tt, vector<MatrixXi> &mtxvec, vector<int> expression)
    {
      vector<string> tt_tmp;
      vector<MatrixXi> mtx_tmp;
      vector<int> exp_tmp;
      vector<string> result;
      string tmp2 = "END";
      expression.erase(expression.begin(), expression.begin() + 2);
      for (int i = 0; i < tt.size(); i++)
      {
        if (tt[i] == "END")
        {
          stp_cut(tt_tmp, mtx_tmp);
          for (int j = 0; j < expression.size(); j++)
          {
            if (expression[0] == 0)
            {
              expression.erase(expression.begin());
              break;
            }
            else
            {
              exp_tmp.push_back(expression[0]);
              expression.erase(expression.begin());
            }
          }
          for (int m = 0; m < tt_tmp.size(); m++)
          {
            result.push_back(tt_tmp[m]);
          }
          result.push_back(tmp2);
          tt_tmp.clear();
          mtx_tmp.clear();
          exp_tmp.clear();
        }
        else
        {
          tt_tmp.push_back(tt[i]);
          mtx_tmp.push_back(mtxvec[i]);
        }
      }
      tt.clear();
      tt.assign(result.begin(), result.end());
    }

    void stp_result(vector<string> &tt, vector<int> &expression)
    {
      int v = expression[0];
      string temp(v, '2');
      expression.erase(expression.begin(), (expression.begin() + 2));
      vector<string> result_tmp;
      vector<string> result;
      result_tmp.push_back(temp);
      for (int i = 0; i < tt.size(); i++)
      {
        if (tt[i] != "END")
        {
          for (int j = 0; j < result_tmp.size(); j++)
          {
            string tmp0 = result_tmp[j];
            for (int l = 0; l < expression.size(); l++)
            {
              if (expression[l] == 0)
              {
                result.push_back(tmp0);
                break;
              }
              else
              {
                int temp_count = expression[l] - 1;
                if ((tt[i][l] == result_tmp[j][temp_count]) || (result_tmp[j][temp_count] == '2'))
                {
                  tmp0[temp_count] = tt[i][l];
                }
                else
                {
                  break;
                }
              }
            }
          }
        }
        else
        {
          result_tmp.clear();
          result_tmp.assign(result.begin(), result.end());
          result.clear();
          for (int i = 0; i < expression.size(); i++)
          {
            if (expression[0] == 0)
            {
              expression.erase(expression.begin());
              break;
            }
            expression.erase(expression.begin());
          }
        }
      }
      tt.clear();
      tt.assign(result_tmp.begin(), result_tmp.end());
    }

    void stp_cut(vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
      vector<MatrixXi> mtx_tmp;
      vector<string> tt_tmp;
      vector<MatrixXi> result_b;
      string stp_result;
      vector<string> result;
      int count = 0;
      int length = tt.size();
      for (int i = 0; i < length; i++) //计算每个子式的变量个数
      {
        if (tt[i] == "A")
        {
          count += 1;
        }
      }
      for (int i = 0; i < length; i++) // CUT计算
      {
        tt_tmp.push_back(tt[0]);
        mtx_tmp.push_back(mtxvec[0]);
        if ((tt[0] != "MN") && (tt[0] != "MD") && (tt[0] != "MP"))
        {
          tt_tmp.pop_back();
          mtx_tmp.pop_back();
          if (tt_tmp.size() >= 1)
          {
            stp_exchange_judge(tt_tmp, mtx_tmp);
            stp_product_judge(tt_tmp, mtx_tmp);
            result_b.push_back(mtx_tmp[0]);
          }
          tt_tmp.clear();
          mtx_tmp.clear();
        }
        tt.erase(tt.begin());
        mtxvec.erase(mtxvec.begin());
      }
      mtxvec.clear();
      mtxvec.assign(result_b.begin(), result_b.end());
      int target = 1;
      stp_result_enumeration(mtxvec, target, stp_result);
      string tmp;
      for (int j = 0; j < stp_result.size(); j++)
      {
        if (stp_result[j] == '\n')
        {
          if (tmp.size() == count)
          {
            result.push_back(tmp);
          }
          if (tmp.size() > count)
          {
            int tmp0 = tmp.size() - count - 1;
            tmp.erase((count - tmp0), tmp0 + 1);
            result.push_back(tmp);
          }
        }
        tmp += stp_result[j];
      }
      tt.clear();
      tt.assign(result.begin(), result.end());
    }

    bool stp_exchange_judge(vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
      for (int i = tt.size(); i > 0; i--)
      {
        for (int j = tt.size(); j > 1; j--)
        {
          if (((tt[j - 1] != "MN") && (tt[j - 1] != "MC") && (tt[j - 1] != "MD") && (tt[j - 1] != "ME") && (tt[j - 1] != "MI") && (tt[j - 1] != "MR") && (tt[j - 1] != "MW")) && ((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")))
          {
            string tmp1_tt;
            string tmp2_tt;
            tmp1_tt += tt[j - 2];
            tmp2_tt += tt[j - 1];
            if (tmp1_tt[0] > tmp2_tt[0])
            {
              MatrixXi matrix_w2(4, 4);
              matrix_w2(0, 0) = 1;
              matrix_w2(0, 1) = 0;
              matrix_w2(0, 2) = 0;
              matrix_w2(0, 3) = 0;
              matrix_w2(1, 0) = 0;
              matrix_w2(1, 1) = 0;
              matrix_w2(1, 2) = 1;
              matrix_w2(1, 3) = 0;
              matrix_w2(2, 0) = 0;
              matrix_w2(2, 1) = 1;
              matrix_w2(2, 2) = 0;
              matrix_w2(2, 3) = 0;
              matrix_w2(3, 0) = 0;
              matrix_w2(3, 1) = 0;
              matrix_w2(3, 2) = 0;
              matrix_w2(3, 3) = 1;
              string tmp3_tt = "MW";
              string tmp4_tt = tt[j - 2];
              tt.insert(tt.begin() + (j - 2), tt[j - 1]);
              tt.erase(tt.begin() + (j - 1));
              tt.insert(tt.begin() + (j - 1), tmp4_tt);
              tt.erase(tt.begin() + j);
              tt.insert(tt.begin() + (j - 2), tmp3_tt);
              MatrixXi tmp_mtx = mtxvec[j - 2];
              mtxvec.insert(mtxvec.begin() + (j - 2), mtxvec[j - 1]);
              mtxvec.erase(mtxvec.begin() + (j - 1));
              mtxvec.insert(mtxvec.begin() + (j - 1), tmp_mtx);
              mtxvec.erase(mtxvec.begin() + j);
              mtxvec.insert(mtxvec.begin() + (j - 2), matrix_w2);
            }
          }
          if (((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")) && ((tt[j - 1] == "MN") || (tt[j - 1] == "MC") || (tt[j - 1] == "MD") || (tt[j - 1] == "ME") || (tt[j - 1] == "MI") || (tt[j - 1] == "MR") || (tt[j - 1] == "MW")))
          {
            MatrixXi tmp1;
            tmp1 = mtxvec[j - 2];
            MatrixXi tmp2;
            tmp2 = mtxvec[j - 1];
            stpm_exchange(tmp1, tmp2);
            string tmp_tt = tt[j - 2];
            tt.insert(tt.begin() + (j - 2), tt[j - 1]);
            tt.erase(tt.begin() + (j - 1));
            tt.insert(tt.begin() + (j - 1), tmp_tt);
            tt.erase(tt.begin() + j);
            mtxvec.insert(mtxvec.begin() + (j - 2), tmp1);
            mtxvec.erase(mtxvec.begin() + (j - 1));
            mtxvec.insert(mtxvec.begin() + (j - 1), tmp2);
            mtxvec.erase(mtxvec.begin() + j);
          }
          if ((tt[j - 2] == tt[j - 1]) && ((tt[j - 2] != "MN") && (tt[j - 2] != "MC") && (tt[j - 2] != "MD") && (tt[j - 2] != "ME") && (tt[j - 2] != "MI") && (tt[j - 2] != "MR") && (tt[j - 2] != "MW")))
          {
            MatrixXi temp_mtx(4, 2);
            temp_mtx(0, 0) = 1;
            temp_mtx(0, 1) = 0;
            temp_mtx(1, 0) = 0;
            temp_mtx(1, 1) = 0;
            temp_mtx(2, 0) = 0;
            temp_mtx(2, 1) = 0;
            temp_mtx(3, 0) = 0;
            temp_mtx(3, 1) = 1;
            mtxvec.insert(mtxvec.begin() + (j - 2), temp_mtx);
            mtxvec.erase(mtxvec.begin() + (j - 1));
            tt.insert(tt.begin() + (j - 2), "MR");
            tt.erase(tt.begin() + (j - 1));
          }
        }
      }
      return true;
    }

    bool stpm_exchange(MatrixXi &matrix_f, MatrixXi &matrix_b)
    {
      MatrixXi exchange_matrix;
      MatrixXi matrix_i;
      exchange_matrix = matrix_b;
      MatrixXi matrix_tmp(2, 2);
      matrix_tmp(0, 0) = 1;
      matrix_tmp(0, 1) = 0;
      matrix_tmp(1, 0) = 0;
      matrix_tmp(1, 1) = 1;
      matrix_i = matrix_tmp;
      matrix_b = matrix_f;
      matrix_f = stp_kron_product(matrix_i, exchange_matrix);
      return true;
    }

    bool stp_product_judge(vector<string> &tt, vector<MatrixXi> &mtxvec)
    {
      MatrixXi temp0, temp1;
      for (int ix = 1; ix < tt.size(); ix++)
      {
        if ((tt[ix] == "MW") || (tt[ix] == "MN") || (tt[ix] == "MC") || (tt[ix] == "MD") || (tt[ix] == "ME") || (tt[ix] == "MI") || (tt[ix] == "MR") || (tt[ix] == "MM"))
        {
          temp0 = mtxvec[0];
          temp1 = mtxvec[ix];
          mtxvec[0] = stpm_basic_product(temp0, temp1);
        }
      }
      return true;
    }

    MatrixXi stpm_basic_product(MatrixXi matrix_f, MatrixXi matrix_b)
    {
      int z;
      MatrixXi result_matrix;
      int n_col = matrix_f.cols();
      int p_row = matrix_b.rows();
      if (n_col % p_row == 0)
      {
        z = n_col / p_row;
        MatrixXi matrix_i1;
        matrix_i1 = MatrixXi::eye(z);
        MatrixXi temp = stp_kron_product(matrix_b, matrix_i1);
        result_matrix = MatrixXi::product(matrix_f, temp);
      }
      else if (p_row % n_col == 0)
      {
        z = p_row / n_col;
        MatrixXi matrix_i2;
        matrix_i2 = MatrixXi::eye(z);
        MatrixXi temp = stp_kron_product(matrix_f, matrix_i2);
        result_matrix = MatrixXi::product(temp, matrix_b);
      }
      return result_matrix;
    }

    MatrixXi stp_kron_product(MatrixXi matrix_f, MatrixXi matrix_b)
    {
      int m = matrix_f.rows();
      int n = matrix_f.cols();
      int p = matrix_b.rows();
      int q = matrix_b.cols();
      MatrixXi dynamic_matrix(m * p, n * q);
      for (int i = 0; i < m * p; i++)
      {
        for (int j = 0; j < n * q; j++)
        {
          dynamic_matrix(i, j) = matrix_f(i / p, j / q) * matrix_b(i % p, j % q);
        }
      }
      return dynamic_matrix;
    }

    void stp_result_enumeration(vector<MatrixXi> &mtxvec, int &target, string &stp_result)
    {
      int target_tmp;
      int n = mtxvec[0].cols();
      target_tmp = target;
      if (n == 4)
      {
        for (int j = 0; j < n; j++)
        {
          if (mtxvec[0](0, j) == target_tmp)
          {
            if (j < 2)
            {
              string tmp = "1";
              stp_result += tmp;
              if (j < 1)
              {
                target = 1;
                if (mtxvec.size() > 1)
                {
                  vector<MatrixXi> temp;
                  temp.assign(mtxvec.begin() + 1, mtxvec.end());
                  stp_result_enumeration(temp, target, stp_result);
                }
                if (mtxvec.size() == 1)
                {
                  string tmp = "1";
                  stp_result += tmp;
                  string tmp1 = "\n";
                  stp_result += tmp1;
                }
              }
              if (j >= 1)
              {
                target = 0;
                if (mtxvec.size() > 1)
                {
                  vector<MatrixXi> temp;
                  temp.assign(mtxvec.begin() + 1, mtxvec.end());
                  stp_result_enumeration(temp, target, stp_result);
                }
                if (mtxvec.size() == 1)
                {
                  string tmp = "0";
                  stp_result += tmp;
                  string tmp1 = "\n";
                  stp_result += tmp1;
                }
              }
            }
            if (j >= 2)
            {
              string tmp = "0";
              stp_result += tmp;
              if (j < ((3 * n) / 4))
              {
                target = 1;
                if (mtxvec.size() > 1)
                {
                  vector<MatrixXi> temp;
                  temp.assign(mtxvec.begin() + 1, mtxvec.end());
                  stp_result_enumeration(temp, target, stp_result);
                }
                if (mtxvec.size() == 1)
                {
                  string tmp = "1";
                  stp_result += tmp;
                  string tmp1 = "\n";
                  stp_result += tmp1;
                }
              }
              if (j >= 3)
              {
                target = 0;
                if (mtxvec.size() > 1)
                {
                  vector<MatrixXi> temp;
                  temp.assign(mtxvec.begin() + 1, mtxvec.end());
                  stp_result_enumeration(temp, target, stp_result);
                }
                if (mtxvec.size() == 1)
                {
                  string tmp = "0";
                  stp_result += tmp;
                  string tmp1 = "\n";
                  stp_result += tmp1;
                }
              }
            }
          }
        }
      }
      else if (n == 2)
      {
        for (int j = 0; j < n; j++)
        {
          if (mtxvec[0](0, j) == target_tmp)
          {
            if (j < 1)
            {
              string tmp = "1";
              stp_result += tmp;
              string tmp1 = "\n";
              stp_result += tmp1;
            }
            if (j >= 1)
            {
              string tmp = "0";
              stp_result += tmp;
              string tmp1 = "\n";
              stp_result += tmp1;
            }
          }
        }
      }
    }

  private:
    vector<int> &expression;
    vector<string> &tt;
    vector<MatrixXi> &mtxvec;
  };

  void cdccl_for_all(vector<string> &in, vector<vector<int>> &mtxvec)
  {
    stp_cdccl_impl p(in, mtxvec);
    p.run_normal();
  }

  void cdccl_for_single_po(vector<string> &in, vector<vector<int>> &mtxvec, int &po)
  {
    stp_cdccl_impl2 p2(in, mtxvec, po);
    p2.run_single_po();
  }

  void stp_cnf(vector<int> &expression, vector<string> &tt, vector<MatrixXi> &mtxvec)
  {
    stp_cnf_impl p3(expression, tt, mtxvec);
    p3.run_cnf();
  }
}
