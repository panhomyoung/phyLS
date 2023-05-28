#ifndef STP_VECTOR_HPP
#define STP_VECTOR_HPP

#include <iostream>
#include <list>
#include <vector>

#include "myfunction.hpp"

namespace phyLS {
class stp_vec;
using word = unsigned;
using m_chain = std::vector<stp_vec>;

class stp_vec {
  // 重载打印操作符
  friend std::ostream &operator<<(std::ostream &os, const stp_vec &v) {
    const unsigned length = v.cols();
    for (unsigned i = 0; i < length; ++i) os << v.vec[i] << " ";
    os << std::endl;
    return os;
  }

  // 重载初始化操作符
  friend std::istream &operator>>(std::istream &is, stp_vec &v) {
    const unsigned length = v.cols();
    for (unsigned i = 0; i < length; ++i) is >> v.vec[i];
    return is;
  }

 public:
  // 各种初始化操作
  stp_vec() { this->vec.clear(); }
  stp_vec(unsigned cols, unsigned value = 0) { this->vec.resize(cols, value); }
  stp_vec(const stp_vec &v) { this->vec = v.vec; }
  stp_vec &operator=(const stp_vec &v) {
    this->vec = v.vec;
    return *this;
  }

  // 重载==
  bool operator==(const stp_vec &v) {
    unsigned m_length = this->cols();
    unsigned length = v.cols();
    if (m_length != length) return false;
    for (unsigned i = 0; i < length; ++i) {
      if (this->vec[i] != v.vec[i]) {
        return false;
      }
    }
    return true;
  }
  // 重载()
  word &operator()(unsigned idx) { return vec[idx]; }

  const word &operator()(unsigned idx) const { return vec[idx]; }

  // 获取矩阵的列数
  unsigned cols() const { return this->vec.size(); }

  // 判断是不是一个变量
  bool is_variable() { return this->cols() == 1; }

  // 块赋值
  bool block(const stp_vec &v, int m_begin, int begin, int num) {
    if (this->cols() < num || v.cols() < num) {
      std::cout << "block abnormal" << std::endl;
      return false;
    }
    for (unsigned i = 0; i < num; ++i) {
      this->vec[m_begin + i] = v.vec[begin + i];
    }
    return true;
  }

 private:
 private:
  std::vector<word> vec;  // 可以对比一下vector 与 deque
};

class stp_logic_manage {
 public:
  // 生成一个交换矩阵
  stp_vec logic_swap_matrix(int m, int n) {
    stp_vec result(m * n + 1);
    int p, q;
    result(0) = m * n;
    for (int i = 0; i < m * n; i++) {
      p = i / m;
      q = i % m;
      int j = q * n + p;
      result(j + 1) = i;
    }
    return result;
  }

  // 克罗内克积
  stp_vec logic_kron_product(const stp_vec &v_f, const stp_vec &v_b) {
    int m = v_f(0);
    int n = v_f.cols() - 1;
    int p = v_b(0);
    int q = v_b.cols() - 1;
    stp_vec result(n * q + 1);
    result(0) = m * p;
    for (int i = 0; i < n; i++) {
      int temp = v_f(i + 1) * p;
      int idx = i * q + 1;
      for (int j = 0; j < q; j++) {
        result(idx + j) = temp + v_b(j + 1);
      }
    }
    return result;
  }

  // 两矩阵乘法
  stp_vec logic_stpm_product(const stp_vec &v_f, const stp_vec &v_b) {
    int mf_row = v_f(0);          // m
    int mf_col = v_f.cols() - 1;  // n
    int mb_row = v_b(0);          // p
    int mb_col = v_b.cols() - 1;  // q
    int row, col;
    stp_vec result;
    if (mf_col % mb_row == 0) {
      int times = mf_col / mb_row;
      row = mf_row;
      col = times * mb_col;
      stp_vec result_matrix(col + 1);
      result_matrix(0) = row;
      for (int i = 1; i <= mb_col; ++i) {
        result_matrix.block(v_f, 1 + times * (i - 1), times * v_b(i) + 1,
                            times);
      }
      return result_matrix;
    } else if (mb_row % mf_col == 0) {
      int times = mb_row / mf_col;
      stp_vec i_times(times + 1);
      i_times(0) = times;
      // 单位矩阵的编码向量
      for (int i = 1; i <= times; i++) {
        i_times(i) = i - 1;
      }
      return logic_stpm_product(logic_kron_product(v_f, i_times), v_b);
    } else {
      std::cout << v_f << v_b << std::endl;
      std::cout << "matrix type error!" << std::endl;
    }
    return result;
  }

  // 矩阵链乘法
  stp_vec matrix_chain_mul(const m_chain &matrix_chain) {
    if (matrix_chain.size() == 1) return matrix_chain[0];
    stp_vec result;
    result = logic_stpm_product(matrix_chain[0], matrix_chain[1]);
    for (int i = 2; i < matrix_chain.size(); i++) {
      result = logic_stpm_product(result, matrix_chain[i]);
    }
    return result;
  }

  // 化为规范型  提前给出变量的信息
  stp_vec normalize_matrix(m_chain &vec_chain) {
    stp_vec Mr(3);
    Mr(0) = 4;
    Mr(1) = 0;
    Mr(2) = 3;  // Reduced power matrix
    stp_vec I2(3);
    I2(0) = 2;
    I2(1) = 0;
    I2(2) = 1;
    stp_vec normal_matrix;
    int p_variable;
    int p;
    int max = 0;
    for (int i = 0; i < vec_chain.size();
         i++)  // the max is the number of variable
    {
      if (vec_chain[i].is_variable() && vec_chain[i](0) > max) {
        max = vec_chain[i](0);
      }
    }
    // 处理直接就是规范型的矩阵链
    if (vec_chain.size() == max + 1) return vec_chain[0];
    // 常规处理
    std::vector<int> idx(max + 1);  // id0 is the max of idx
    p_variable = vec_chain.size() - 1;
    int flag;
    while (p_variable >= 0) {
      int flag = 0;
      if (vec_chain[p_variable].is_variable())  // 1:find a variable
      {
        if (idx[vec_chain[p_variable](0)] == 0) {
          idx[vec_chain[p_variable](0)] = idx[0] + 1;
          idx[0]++;
          if (p_variable == vec_chain.size() - 1)  //!
          {
            vec_chain.pop_back();
            p_variable--;
            continue;
          }
        } else {
          if (idx[vec_chain[p_variable](0)] == idx[0]) {
            flag = 1;
            if (p_variable == vec_chain.size() - 1) {
              vec_chain.pop_back();
              vec_chain.push_back(Mr);
              continue;
            }
          } else {
            flag = 1;
            vec_chain.push_back(logic_swap_matrix(
                2, 1 << (idx[0] - idx[vec_chain[p_variable](0)])));
            for (int i = 1; i <= max; i++) {
              if (idx[i] != 0 && idx[i] > idx[vec_chain[p_variable](0)])
                idx[i]--;
            }
            idx[vec_chain[p_variable](0)] = idx[0];
          }
        }
        m_chain matrix_chain1;  //?
        matrix_chain1.clear();
        for (p = p_variable + 1; p < vec_chain.size(); p++) {
          matrix_chain1.push_back(vec_chain[p]);  // have no matrix
        }
        while (p > p_variable + 1) {
          vec_chain.pop_back();
          p--;
        }
        if (matrix_chain1.size() > 0) {
          vec_chain.push_back(matrix_chain_mul(matrix_chain1));
        }
        if (p_variable != vec_chain.size() - 1) {
          vec_chain[p_variable] =
              logic_kron_product(I2, vec_chain[p_variable + 1]);
          vec_chain.pop_back();
        }
        if (flag) vec_chain.push_back(Mr);
        continue;
      } else {
        p_variable--;
      }
    }
    // 后续矩阵链中已经没有变量
    for (int i = max; i > 0; i--)  //!
    {
      vec_chain.push_back(logic_swap_matrix(2, 1 << (idx[0] - idx[i])));
      for (int j = 1; j <= max; j++) {
        if (idx[j] != 0 && idx[j] > idx[i]) idx[j]--;
      }
      idx[i] = max;
    }
    normal_matrix = matrix_chain_mul(vec_chain);
    return normal_matrix;
  }

 private:
};
}  // namespace phyLS

#endif