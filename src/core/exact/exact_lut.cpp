#include "exact_lut.hpp"

using namespace std;

namespace phyLS {
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
  string tt = "2222";
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
  string name;
  bool input = 0;
};

struct result_lut {
  string computed_input;
  vector<bench> possible_result;
};

struct coordinate {
  int Abscissa;
  int Ordinate;
  int parameter_Intermediate;
  int parameter_Gate;
};

struct cdccl_impl {
  vector<string> Intermediate;
  string Result;
  vector<int> Gate;
  vector<coordinate> Level;  // Define the network as a coordinate system
};

class exact_lut_impl {
 public:
  exact_lut_impl(vector<string>& tt, int& input) : tt(tt), input(input) {}

  void run() { exact_lut_network(); }

  void run_enu() { exact_lut_network_enu(); }

 private:
  void exact_lut_network() {
    // the DAG topology family of r nodes and k levels
    // initialization
    int num_node = input - 1;  // first, number of node is number of input - 1
    int num_level = 2;
    while (1) {
      vector<vector<bench>> lut;
      create_dags(lut, num_node, num_level);

      if (lut.empty()) {
        if (num_level <= num_node) {
          num_level += 1;
        }
        if (num_level > num_node) {
          num_level = 2;
          num_node += 1;
        }
        continue;
      }
      // AllSAT solving to judge the DAGs
      stp_simulate(lut);
      if (lut.size()) {
        vector<string> result_final;
        for (int i = 0; i < lut.size(); i++) {
          string result;
          for (int j = 0; j < lut[i].size(); j++) {
            result += to_string(lut[i][j].node);
            result += " = 4'b";
            result += lut[i][j].tt;
            result += " (";
            if (lut[i][j].left <= input) {
              char temp;
              temp = 'a' + lut[i][j].left - 1;
              result.push_back(temp);
            } else {
              result += to_string(lut[i][j].left);
            }
            result += ", ";
            if (lut[i][j].right <= input) {
              char temp;
              temp = 'a' + lut[i][j].right - 1;
              result.push_back(temp);
            } else {
              result += to_string(lut[i][j].right);
            }
            result += ")  ";
          }
          result_final.push_back(result);
        }
        tt.assign(result_final.begin(), result_final.end());
        break;
      } else {
        if (num_level <= num_node) {
          num_level += 1;
        }
        if (num_level > num_node) {
          num_level = 2;
          num_node += 1;
        }
      }
    }
  }

  void exact_lut_network_enu() {
    // the DAG topology family of r nodes and k levels
    // initialization
    int num_node = input - 1;  // first, number of node is number of input - 1
    int num_level = 2;
    while (1) {
      vector<vector<bench>> lut;
      create_dags(lut, num_node, num_level);

      if (lut.empty()) {
        if (num_level <= num_node) {
          num_level += 1;
        }
        if (num_level > num_node) {
          num_level = 2;
          num_node += 1;
        }
        continue;
      }
      // AllSAT solving to judge the DAGs
      stp_simulate_enu(lut);
      if (lut.size()) {
        vector<string> result_final;
        for (int i = 0; i < lut.size(); i++) {
          string result;
          for (int j = 0; j < lut[i].size(); j++) {
            result += to_string(lut[i][j].node);
            result += " = 4'b";
            result += lut[i][j].tt;
            result += " (";
            if (lut[i][j].left <= input) {
              char temp;
              temp = 'a' + lut[i][j].left - 1;
              result.push_back(temp);
            } else {
              result += to_string(lut[i][j].left);
            }
            result += ", ";
            if (lut[i][j].right <= input) {
              char temp;
              temp = 'a' + lut[i][j].right - 1;
              result.push_back(temp);
            } else {
              result += to_string(lut[i][j].right);
            }
            result += ")  ";
          }
          result_final.push_back(result);
        }
        tt.assign(result_final.begin(), result_final.end());
        break;
      } else {
        if (num_level <= num_node) {
          num_level += 1;
        }
        if (num_level > num_node) {
          num_level = 2;
          num_node += 1;
        }
      }
    }
  }

  void create_dags(vector<vector<bench>>& lut_dags, int node, int level) {
    dag lut_dag_init;
    lut_dag_init.node = node;
    lut_dag_init.level = level;
    lut_dag_init.count.resize(level + 1, 1);
    lut_dag_init.count[0] = input;
    int numnodes = node - level;
    int numlevels = level - 1;
    vector<vector<int>> q;
    if (numlevels > 0) {
      if (numnodes == 0) {
        vector<vector<bench>> lut_dags_temp;
        bench_dag(lut_dag_init, lut_dags_temp);
        lut_dags.insert(lut_dags.end(), lut_dags_temp.begin(),
                        lut_dags_temp.end());
      } else {
        dag_classify(q, numnodes, numlevels);
        for (int i = 0; i < q.size(); i++) {
          dag lut_dag_temp = lut_dag_init;
          for (int j = 1, k = 0; j < level; j++, k++) {
            lut_dag_temp.count[j] += q[i][k];
          }
          judge_dag(lut_dag_temp);
          if (lut_dag_temp.count.size() != 0) {
            vector<vector<bench>> lut_dags_temp;
            bench_dag(lut_dag_temp, lut_dags_temp);
            lut_dags.insert(lut_dags.end(), lut_dags_temp.begin(),
                            lut_dags_temp.end());
          }
        }
      }
    }
  }

  void dag_classify(vector<vector<int>>& result, int numnodes, int numlevels) {
    int q[numlevels];
    int x = numnodes;
    int j;
    for (int i = 0; i < numlevels; i++) {
      q[i] = 0;
    }
    while (1) {
      j = 0;
      q[0] = x;
      while (1) {
        vector<int> result_temp;
        for (int i = numlevels - 1; i >= 0; i--) {
          result_temp.push_back(q[i]);
        }
        result.push_back(result_temp);
        if (j == 0) {
          x = q[0] - 1;
          j = 1;
        } else if (q[0] == 0) {
          x = q[j] - 1;
          q[j] = 0;
          j++;
        }
        if (j >= numlevels) {
          return;
        } else if (q[j] == numnodes) {
          x = x + numlevels;
          q[j] = 0;
          j++;
          if (j >= numlevels) {
            return;
          }
        }
        q[j]++;
        if (x == 0) {
          q[0] = 0;
          continue;
        } else {
          break;
        }
      }
    }
  }

  void judge_dag(dag& lut_dag) {
    for (int i = 0; i < lut_dag.count.size(); i++) {
      int sum =
          accumulate(lut_dag.count.begin() + i + 1, lut_dag.count.end(), 0);
      if (lut_dag.count[i] - 1 > sum) {
        lut_dag.count.clear();
        break;
      }
    }
  }

  void bench_dag(dag lut_dag, vector<vector<bench>>& lut_dags) {
    int sum = 0, level_temp = 0, self_temp = 1;
    // 第一个for循环
    // 通过lut_dag的count，初始化lut_dag的lut
    // 编号每一个节点，并且将其放置在对应的层中
    // 每一层就是lut的层
    for (int i = 0; i < lut_dag.count.size(); i++, level_temp++) {
      vector<node_inf> node_vec;
      sum += lut_dag.count[i];
      for (; self_temp < sum + 1; self_temp++) {
        node_inf node;
        node.num = self_temp;
        node.level = level_temp;
        if (self_temp <= lut_dag.count[0]) {
          node.fan_in = 1;
        } else if (i == lut_dag.count.size() - 1) {
          node.fan_out = 1;
        }
        node_vec.push_back(node);
      }
      lut_dag.lut.push_back(node_vec);
    }

    /*
    cout << "node information : " << endl;
    for (auto x : lut_dag.lut)
    {
        for (auto y : x)
        {
            cout << y.num << " " << y.level << " " << y.fan_in << " " <<
    y.fan_out << endl;
        }
    }
    */

    // 第二个for循环
    // 生成所有满足的DAG结构
    for (int i = lut_dag.lut.size() - 1; i >= 0; i--) {
      vector<bench> lut_dags_mid;
      vector<int> permutation;
      // 第2.1个for循环
      // 从lut的最上层开始向下进行遍历
      // 若为输出节点，则直接构造节点，不排列组合并放入结果中
      // 若为输入节点，不构造，直接进行排列组合(permutation)
      // 否则，先构造，再排列组合
      for (int j = lut_dag.lut[i].size() - 1; j >= 0; j--) {
        if (lut_dag.lut[i][j].fan_out) {
          bench LUT_fanout;
          LUT_fanout.node = lut_dag.lut[i][j].num;
          vector<bench> lut_dags_fanout;
          lut_dags_fanout.push_back(LUT_fanout);
          lut_dags.push_back(lut_dags_fanout);
        } else if (lut_dag.lut[i][j].fan_in) {
          permutation.push_back(lut_dag.lut[i][j].num);
        } else {
          bench LUT_mid;
          LUT_mid.node = lut_dag.lut[i][j].num;
          lut_dags_mid.push_back(LUT_mid);
          permutation.push_back(lut_dag.lut[i][j].num);
        }
      }
      // 排列组合算法
      // 仅针对中间节点进行排列组合
      if (permutation.size()) {
        vector<vector<bench>> lut_dags_result;
        // 第2.2个for循环
        // 遍历已存在的所有结果(lut_dags)
        for (int k = 0; k < lut_dags.size(); k++) {
          vector<bench> lut_dags_rst_temp;  // 尚未满足的节点
          vector<bench> lut_dags_rst;       // 已满足的节点
          vector<int> count;
          // 第2.2.1个for循环
          // 为排列组合做准备
          // 判断还有哪些节点未填满，并确定剩余空间
          for (int l = 0; l < lut_dags[k].size(); l++) {
            if (lut_dags[k][l].left == 0 || lut_dags[k][l].right == 0) {
              int space_temp = 0;
              if (lut_dags[k][l].right == 0) {
                space_temp += 1;
              }
              if (lut_dags[k][l].left == 0) {
                space_temp += 1;
              }
              count.push_back(space_temp);
              lut_dags_rst_temp.push_back(lut_dags[k][l]);
            } else {
              lut_dags_rst.push_back(lut_dags[k][l]);
            }
          }
          vector<vector<int>> result;
          int numnodes = permutation.size();
          int space = accumulate(count.begin(), count.end(), 0);
          if (space == numnodes) {
            vector<vector<int>> result_temp;
            dag_generate_complete(result, result_temp, permutation, count, 0);
          } else if (space < numnodes) {
            result.clear();
          } else {
            vector<vector<int>> result_temp;
            dag_generate_uncomplete(result, result_temp, permutation,
                                    permutation, count, 0);
          }
          // result有结果
          if (result.size()) {
            if (i == 0) {
              for (int m = 0; m < result.size(); m++) {
                bool target = 1;
                vector<bench> result_temp1;
                int n2 = 0;
                for (int n1 = 0; n1 < lut_dags_rst_temp.size(); n1++) {
                  bench result_temp_temp = lut_dags_rst_temp[n1];
                  if (result_temp_temp.left == 0 &&
                      result_temp_temp.right == 0) {
                    if (result[m][n2] != 0) {
                      result_temp_temp.left = result[m][n2];
                    } else {
                      target = 0;
                    }
                    if (result[m][n2 + 1] != 0) {
                      result_temp_temp.right = result[m][n2 + 1];
                    } else {
                      target = 0;
                    }
                    n2 += 2;
                  } else if (result_temp_temp.left == 0 &&
                             result_temp_temp.right != 0) {
                    if (result[m][n2] != 0) {
                      result_temp_temp.left = result[m][n2];
                    } else {
                      target = 0;
                    }
                    n2 += 1;
                  }
                  if (!target) {
                    break;
                  }
                  result_temp1.push_back(result_temp_temp);
                }
                if (target) {
                  if (lut_dags_mid.size()) {
                    result_temp1.insert(result_temp1.end(),
                                        lut_dags_mid.begin(),
                                        lut_dags_mid.end());
                  }
                  if (lut_dags_rst.size()) {
                    result_temp1.insert(result_temp1.begin(),
                                        lut_dags_rst.begin(),
                                        lut_dags_rst.end());
                  }
                  lut_dags_result.push_back(result_temp1);
                }
              }
            } else {
              for (int m = 0; m < result.size(); m++) {
                vector<bench> result_temp1;
                int n2 = 0;
                for (int n1 = 0; n1 < lut_dags_rst_temp.size(); n1++) {
                  bench result_temp_temp = lut_dags_rst_temp[n1];
                  if (result_temp_temp.left == 0 &&
                      result_temp_temp.right == 0) {
                    result_temp_temp.left = result[m][n2];
                    result_temp_temp.right = result[m][n2 + 1];
                    n2 += 2;
                  } else if (result_temp_temp.left == 0 &&
                             result_temp_temp.right != 0) {
                    result_temp_temp.left = result[m][n2];
                    n2 += 1;
                  }
                  result_temp1.push_back(result_temp_temp);
                }
                if (lut_dags_mid.size()) {
                  result_temp1.insert(result_temp1.end(), lut_dags_mid.begin(),
                                      lut_dags_mid.end());
                }
                if (lut_dags_rst.size()) {
                  result_temp1.insert(result_temp1.begin(),
                                      lut_dags_rst.begin(), lut_dags_rst.end());
                }
                lut_dags_result.push_back(result_temp1);
              }
            }
          }
        }
        lut_dags.assign(lut_dags_result.begin(), lut_dags_result.end());
      }
    }
  }

  void LUT_classify(vector<vector<int>>& result, int numnodes, int numlevels) {
    int q[numlevels];
    int x = numnodes;
    int j;
    for (int i = 0; i < numlevels; i++) {
      q[i] = 0;
    }

    while (1) {
      j = 0;
      q[0] = x;
      while (1) {
        vector<int> result_temp;
        bool target = 0;
        for (int i = numlevels - 1; i >= 0; i--) {
          result_temp.push_back(q[i]);
          if (q[i] > 2) {
            target = 1;
          }
        }
        if (!target) {
          result.push_back(result_temp);
        }
        if (j == 0) {
          x = q[0] - 1;
          j = 1;
        } else if (q[0] == 0) {
          x = q[j] - 1;
          q[j] = 0;
          j++;
        }
        if (j >= numlevels) {
          return;
        } else if (q[j] == numnodes) {
          x = x + numlevels;
          q[j] = 0;
          j++;
          if (j >= numlevels) {
            return;
          }
        }
        q[j]++;
        if (x == 0) {
          q[0] = 0;
          continue;
        } else {
          break;
        }
      }
    }
  }

  void dag_generate_complete(vector<vector<int>>& result,
                             vector<vector<int>>& rst, vector<int> permutation,
                             vector<int> count, int level) {
    vector<vector<int>> result_temp;
    vector<int> q(count[level]);
    combinate(0, 0, permutation.size(), count[level], permutation, q,
              result_temp);
    for (int j = 0; j < result_temp.size(); j++) {
      vector<int> elected(permutation);
      vector<int> result_1;
      for (int k = 0; k < result_temp[j].size(); k++) {
        result_1.push_back(result_temp[j][k]);
        if (level + 1 < count.size()) {
          // 从vector中删除指定的某一个元素
          for (vector<int>::iterator iter = elected.begin();
               iter != elected.end(); iter++) {
            if (*iter == result_temp[j][k]) {
              elected.erase(iter);
              break;
            }
          }
        }
      }
      sort(result_1.begin(), result_1.end());
      if (level + 1 < count.size()) {
        vector<vector<int>> rst_temp;
        dag_generate_complete(result, rst_temp, elected, count, level + 1);
        for (int l = 0; l < rst_temp.size(); l++) {
          vector<int> result_2(result_1);
          result_2.insert(result_2.end(), rst_temp[l].begin(),
                          rst_temp[l].end());
          rst.push_back(result_2);
        }
      } else {
        rst.push_back(result_1);
      }

      if (level == 0) {
        result.insert(result.end(), rst.begin(), rst.end());
        rst.clear();
      }
    }
  }

  void dag_generate_uncomplete(vector<vector<int>>& result,
                               vector<vector<int>>& rst,
                               vector<int> permutation_f,
                               vector<int> permutation_b, vector<int> count,
                               int level) {
    vector<vector<int>> result_temp2;
    for (int i = 0; i <= count[level]; i++) {
      vector<vector<int>> result_temp;
      vector<int> q(count[level]);
      combinate(0, 0, permutation_f.size(), i, permutation_f, q, result_temp);
      result_temp2.insert(result_temp2.end(), result_temp.begin(),
                          result_temp.end());
    }
    for (int j = 0; j < result_temp2.size(); j++) {
      vector<int> permutation_b_b(permutation_b);
      vector<int> result_1;
      for (int k = 0; k < result_temp2[j].size(); k++) {
        result_1.push_back(result_temp2[j][k]);
        // 从vector中删除指定的某一个元素
        for (vector<int>::iterator iter = permutation_b_b.begin();
             iter != permutation_b_b.end(); iter++) {
          if (*iter == result_temp2[j][k]) {
            permutation_b_b.erase(iter);
            break;
          }
        }
      }
      sort(result_1.begin(), result_1.end());
      if (level + 1 < count.size()) {
        vector<vector<int>> rst_temp;
        dag_generate_uncomplete(result, rst_temp, permutation_f,
                                permutation_b_b, count, level + 1);
        for (int l = 0; l < rst_temp.size(); l++) {
          vector<int> result_2(result_1);
          result_2.insert(result_2.end(), rst_temp[l].begin(),
                          rst_temp[l].end());
          rst.push_back(result_2);
        }
      } else {
        if (!permutation_b_b.size()) {
          rst.push_back(result_1);
        }
      }
      if (level == 0) {
        result.insert(result.end(), rst.begin(), rst.end());
        rst.clear();
      }
    }
  }

  void combinate(int iPos, int iProc, int numnodes, int numlevels,
                 vector<int> data, vector<int> des,
                 vector<vector<int>>& result) {
    if (iProc > numnodes) {
      return;
    }
    if (iPos == numlevels) {
      result.push_back(des);
      return;
    } else {
      combinate(iPos, iProc + 1, numnodes, numlevels, data, des, result);
      des[iPos] = data[iProc];
      combinate(iPos + 1, iProc + 1, numnodes, numlevels, data, des, result);
    }
  }

  void stp_simulate(vector<vector<bench>>& lut_dags) {
    string tt_binary = tt[0];
    int flag_node = lut_dags[0][lut_dags[0].size() - 1].node;
    vector<vector<bench>> lut_dags_new;
    for (int i = lut_dags.size() - 1; i >= 0; i--) {
      string input_tt(tt_binary);

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
              string temp1, temp2;
              temp1 = input_tt.substr(length_string / 4, length_string / 4);
              temp2 = input_tt.substr(length_string / 2, length_string / 4);
              input_tt.replace(length_string / 4, length_string / 4, temp2);
              input_tt.replace(length_string / 2, length_string / 4, temp1);
            } else {
              int length2 = length_string / pow(2.0, matrix_form[l].eye + 2);
              for (int l1 = pow(2.0, matrix_form[l].eye), add = 1; l1 > 0;
                   l1--) {
                string temp1, temp2;
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
              string temp(length_string, '2');
              input_tt.insert(input_tt.begin() + (length_string / 2),
                              temp.begin(), temp.end());
            } else {
              string temp(length_string / pow(2.0, matrix_form[l].eye), '2');
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
              string standard(length1, '2');
              vector<int> standard_int(4, 0);
              vector<vector<int>> result;

              for (int l3 = 0; l3 < pow(2.0, matrix_form[l].eye); l3++) {
                vector<vector<int>> result_t;

                int ind = (length_string2 / pow(2.0, matrix_form[l].eye)) * l3;
                string a = bench_result[q].computed_input.substr(ind, length1);
                string b = bench_result[q].computed_input.substr(ind + length1,
                                                                 length1);
                string c = bench_result[q].computed_input.substr(
                    ind + (2 * length1), length1);
                string d = bench_result[q].computed_input.substr(
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
                  string count1, count2;
                  string res1, res2;
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
                    string s1(2 * length1, '2');
                    string s2(2 * length1, '2');
                    int ind =
                        (length_string2 / pow(2.0, matrix_form[l].eye)) * n;
                    string a, b, c, d;
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

      for (int j = bench_result.size() - 1; j >= 0; j--) {
        vector<vector<int>> mtxvec;
        vector<int> mtx1, mtx2;
        vector<string> in;
        for (int k = bench_result[j].possible_result.size() - 1; k >= 0; k--) {
          vector<int> mtxvec_temp;
          in.push_back(bench_result[j].possible_result[k].tt);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].left);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].right);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].node);
          mtxvec.push_back(mtxvec_temp);
        }
        in.push_back("10");
        mtx1.push_back(bench_result[j].possible_result[0].node);
        mtx2.push_back(input);
        mtx2.push_back(1);
        mtxvec.push_back(mtx1);
        mtxvec.push_back(mtx2);
        vector<vector<string>> in_expansion = bench_expansion(in);
        vector<result_lut> bench_temp;
        for (int k = in_expansion.size() - 1; k >= 0; k--) {
          string tt_temp = in_expansion[k][in_expansion[k].size() - 2];
          bench_solve(in_expansion[k], mtxvec);
          if (!compare_result(in_expansion[k], tt_binary)) {
            in_expansion.erase(in_expansion.begin() + k);
          } else {
            result_lut bench_temp_temp = bench_result[j];
            bench_temp_temp.possible_result[0].tt = tt_temp;
            bench_temp.push_back(bench_temp_temp);
          }
        }
        if (in_expansion.empty())
          bench_result.erase(bench_result.begin() + j);
        else {
          bench_result.erase(bench_result.begin() + j);
          bench_result.insert(bench_result.end(), bench_temp.begin(),
                              bench_temp.end());
        }
      }

      if (bench_result.size()) {
        for (int j = 0; j < bench_result.size(); j++)
          lut_dags_new.push_back(bench_result[j].possible_result);
        break;
      }
    }
    lut_dags.assign(lut_dags_new.begin(), lut_dags_new.end());
  }

  void stp_simulate_enu(vector<vector<bench>>& lut_dags) {
    string tt_binary = tt[0];
    int flag_node = lut_dags[0][lut_dags[0].size() - 1].node;
    vector<vector<bench>> lut_dags_new;
    for (int i = lut_dags.size() - 1; i >= 0; i--) {
      string input_tt(tt_binary);

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
              string temp1, temp2;
              temp1 = input_tt.substr(length_string / 4, length_string / 4);
              temp2 = input_tt.substr(length_string / 2, length_string / 4);
              input_tt.replace(length_string / 4, length_string / 4, temp2);
              input_tt.replace(length_string / 2, length_string / 4, temp1);
            } else {
              int length2 = length_string / pow(2.0, matrix_form[l].eye + 2);
              for (int l1 = pow(2.0, matrix_form[l].eye), add = 1; l1 > 0;
                   l1--) {
                string temp1, temp2;
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
              string temp(length_string, '2');
              input_tt.insert(input_tt.begin() + (length_string / 2),
                              temp.begin(), temp.end());
            } else {
              string temp(length_string / pow(2.0, matrix_form[l].eye), '2');
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
              string standard(length1, '2');
              vector<int> standard_int(4, 0);
              vector<vector<int>> result;

              for (int l3 = 0; l3 < pow(2.0, matrix_form[l].eye); l3++) {
                vector<vector<int>> result_t;

                int ind = (length_string2 / pow(2.0, matrix_form[l].eye)) * l3;
                string a = bench_result[q].computed_input.substr(ind, length1);
                string b = bench_result[q].computed_input.substr(ind + length1,
                                                                 length1);
                string c = bench_result[q].computed_input.substr(
                    ind + (2 * length1), length1);
                string d = bench_result[q].computed_input.substr(
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
                  string count1, count2;
                  string res1, res2;
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
                    string s1(2 * length1, '2');
                    string s2(2 * length1, '2');
                    int ind =
                        (length_string2 / pow(2.0, matrix_form[l].eye)) * n;
                    string a, b, c, d;
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

      for (int j = bench_result.size() - 1; j >= 0; j--) {
        vector<vector<int>> mtxvec;
        vector<int> mtx1, mtx2;
        vector<string> in;
        for (int k = bench_result[j].possible_result.size() - 1; k >= 0; k--) {
          vector<int> mtxvec_temp;
          in.push_back(bench_result[j].possible_result[k].tt);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].left);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].right);
          mtxvec_temp.push_back(bench_result[j].possible_result[k].node);
          mtxvec.push_back(mtxvec_temp);
        }
        in.push_back("10");
        mtx1.push_back(bench_result[j].possible_result[0].node);
        mtx2.push_back(input);
        mtx2.push_back(1);
        mtxvec.push_back(mtx1);
        mtxvec.push_back(mtx2);
        vector<vector<string>> in_expansion = bench_expansion(in);
        vector<result_lut> bench_temp;
        for (int k = in_expansion.size() - 1; k >= 0; k--) {
          string tt_temp = in_expansion[k][in_expansion[k].size() - 2];
          bench_solve(in_expansion[k], mtxvec);
          if (!compare_result(in_expansion[k], tt_binary)) {
            in_expansion.erase(in_expansion.begin() + k);
          } else {
            result_lut bench_temp_temp = bench_result[j];
            bench_temp_temp.possible_result[0].tt = tt_temp;
            bench_temp.push_back(bench_temp_temp);
          }
        }
        if (in_expansion.empty())
          bench_result.erase(bench_result.begin() + j);
        else {
          bench_result.erase(bench_result.begin() + j);
          bench_result.insert(bench_result.end(), bench_temp.begin(),
                              bench_temp.end());
        }
      }

      if (bench_result.size()) {
        for (int j = 0; j < bench_result.size(); j++)
          lut_dags_new.push_back(bench_result[j].possible_result);
      }
    }
    lut_dags.assign(lut_dags_new.begin(), lut_dags_new.end());
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
      string str = to_string(lut_dags[0].right);
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
      string str = to_string(lut_dags[0].left);
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
          string str = to_string(lut[i].right);
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
          string str = to_string(lut[i].left);
          temp_left.name = str;
          matrix_form.push_back(temp_left);
        }
      }
    }
  }

  bool compare_string(string a, string b) {
    bool target = 1;
    for (int i = 0; i < a.size(); i++) {
      if ((a[i] != b[i]) && a[i] != '2' && b[i] != '2') {
        target = 0;
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

  bool compare_result(vector<string> t1, string t2) {
    bool target = false;
    string t3(t2.size(), '0');
    for (int i = 0; i < t1.size(); i++) {
      char temp;
      for (int j = 0; j < t1[i].size() / 2; j++) {
        temp = t1[i][j];
        t1[i][j] = t1[i][t1[i].size() - 1 - j];
        t1[i][t1[i].size() - 1 - j] = temp;
      }
      int value = stoi(t1[i], 0, 2);
      int ind = t3.size() - value - 1;
      t3[ind] = '1';
    }
    if (t2 == t3) {
      target = true;
    }
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

  void bench_solve(vector<string>& in, vector<vector<int>> mtxvec) {
    int length1 = mtxvec[mtxvec.size() - 1][0];  // number of primary input
    int length2 = mtxvec[mtxvec.size() - 1][1];  // number of primary output
    int length3 = mtxvec[0][2];                  // the minimum minuend
    int length4 =
        mtxvec[mtxvec.size() - 2 - length2][2];  // the maximum variable
    vector<vector<cdccl_impl>>
        list;  // the solution space is two dimension vector
    vector<cdccl_impl> list_temp;
    cdccl_impl list_temp_temp;

    string Result_temp(length1, '2');  // temporary result
    coordinate Level_temp;
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
    coordinate Gate_level;
    Gate_level.Abscissa = 0;
    Gate_level.Ordinate = 0;
    Gate_level.parameter_Intermediate = -1;
    Gate_level.parameter_Gate = -1;
    string Intermediate_temp;
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
      vector<cdccl_impl> list_temp1;
      for (int k = 0; k < list[level].size(); k++) {
        cdccl_impl temp1;                      // temporary gate
        temp1.Result = list[level][k].Result;  // first, next level's Result is
                                               // same as the front level
        for (int j = 0; j < list[level][k].Intermediate.size(); j++) {
          temp1.Level = list[level][k].Level;  // next level's Level information
          for (int j1 = 0; j1 < list[level][k].Gate.size(); j1++) {
            temp1.Level[list[level][k].Gate[j1] - 1].parameter_Intermediate = j;
            temp1.Level[list[level][k].Gate[j1] - 1].parameter_Gate = j1;
          }

          coordinate level_current;  // new level
          level_current.Abscissa = level + 1;
          level_current.parameter_Intermediate = -1;
          level_current.parameter_Gate = -1;
          level_current.Ordinate = ordinate;

          temp1.Gate.assign(list[level][k].Gate.begin(),
                            list[level][k].Gate.end());  // need more!!!!!
          temp1.Intermediate.push_back(list[level][k].Intermediate[j]);

          vector<string> Intermediate_temp;
          vector<int> Gate_temp;
          vector<int> Gate_judge(length4, -1);
          int count_cdccl = 0;
          for (int i = 0; i < temp1.Gate.size(); i++) {
            int length = temp1.Gate[i] - length3;
            int Gate_f = mtxvec[length][0];  // the front Gate variable
            int Gate_b = mtxvec[length][1];  // the behind Gate variable
            string tt = in[length];          // the correndsponding Truth Table
            char target = temp1.Intermediate[0][i];  // the SAT target
            vector<string> Intermediate_temp_temp;

            int flag_cdccl = 0;
            string Intermediate_temp_temp_F, Intermediate_temp_temp_B;
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
                  string t1, t2, t3, t4;
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
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t1;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t2 == "0") {
                        t2 += "1";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t2;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t3 == "1") {
                        t3 += "0";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t3;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t4 == "0") {
                        t4 += "0";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t4;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                    }
                    if (flag == 2) {
                      if (t1 == "11") {
                        t1 = "1";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t1;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t2 == "01") {
                        t2 = "1";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t2;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t3 == "10") {
                        t3 = "0";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t3;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                      if (t4 == "00") {
                        t4 = "0";
                        string result_temporary(Intermediate_temp[j]);
                        result_temporary += t4;
                        Intermediate_temp_temp.push_back(result_temporary);
                      }
                    }
                  } else {
                    if (flag == 1) {
                      if (tt[0] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '1') {
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t1;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[1] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '1') {
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t2;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[2] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '0') {
                          string result_temporary(Intermediate_temp[j]);
                          result_temporary += t3;
                          Intermediate_temp_temp.push_back(result_temporary);
                        }
                      }
                      if (tt[3] == target) {
                        if (Intermediate_temp[j][Gate_judge[Gate_b - 1]] ==
                            '0') {
                          string result_temporary(Intermediate_temp[j]);
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
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[1] == target) {
                      if (Intermediate_temp_temp_F == "0") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[2] == target) {
                      if (Intermediate_temp_temp_F == "1") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[3] == target) {
                      if (Intermediate_temp_temp_F == "0") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
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
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[1] == target) {
                      if (Intermediate_temp_temp_B == "1") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "0";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[2] == target) {
                      if (Intermediate_temp_temp_B == "0") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
                        Intermediate_temp_temp1 += "1";
                        Intermediate_temp_temp.push_back(
                            Intermediate_temp_temp1);
                      }
                    }
                    if (tt[3] == target) {
                      if (Intermediate_temp_temp_B == "0") {
                        string Intermediate_temp_temp1(Intermediate_temp[j]);
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
            cdccl_impl temp2;
            temp2.Level = temp1.Level;
            string Result_temp(temp1.Result);
            temp2.Gate.assign(temp1.Gate.begin(), temp1.Gate.end());
            string Intermediate_temp1(temp1.Intermediate[k]);
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

  vector<vector<string>> bench_expansion(vector<string> in) {
    vector<vector<string>> in_expansion;
    in_expansion.push_back(in);
    string in_end = in[in.size() - 2];
    for (int i = 0; i < in_end.size(); i++) {
      if (in_end[i] == '2') {
        vector<vector<string>> in_expansion_temp;
        for (int j = 0; j < in_expansion.size(); j++) {
          vector<string> in_temp_1(in_expansion[j]);
          vector<string> in_temp_0(in_expansion[j]);
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
  vector<string>& tt;
  int& input;
};

void exact_lut(vector<string>& tt, int& input) {
  exact_lut_impl p(tt, input);
  p.run();
}

void exact_lut_enu(vector<string>& tt, int& input) {
  exact_lut_impl p(tt, input);
  p.run_enu();
}
}  // namespace phyLS
