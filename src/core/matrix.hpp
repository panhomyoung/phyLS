#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

class MatrixXi {
  friend ostream &operator<<(ostream &os, const MatrixXi &m) {
    for (size_t i = 0; i < m.row; i++) {
      for (size_t j = 0; j < m.col; j++) {
        os << m.data[i][j] << " ";
      }
      os << endl;
    }
    return os;
  }

  friend istream &operator>>(istream &is, MatrixXi &m) {
    for (size_t i = 0; i < m.row; i++) {
      for (size_t j = 0; j < m.col; j++) {
        is >> m.data[i][j];
      }
    }
    return is;
  }

 public:
  typedef int value_type;
  typedef vector<int>::size_type size_type;

  MatrixXi() {
    row = 0;
    col = 0;
    data.clear();
  }
  MatrixXi(size_t i, size_t j) {
    row = i;
    col = j;
    vector<vector<int>> vdata(row, vector<int>(col, 0));
    data = move(vdata);
  }

  MatrixXi(const MatrixXi &m) {
    row = m.row;
    col = m.col;
    data = m.data;
  }

  MatrixXi &operator=(const MatrixXi &m) {
    row = m.row;
    col = m.col;
    data = m.data;
    return *this;
  }

  static MatrixXi product(MatrixXi &m, MatrixXi &n) {
    MatrixXi t(m.row, n.col);
    for (size_t i = 0; i < m.row; i++) {
      for (size_t j = 0; j < n.col; j++) {
        for (size_t k = 0; k < m.col; k++) {
          t.data[i][j] += (m.data[i][k] * n.data[k][j]);
        }
      }
    }
    return t;
  }

  static MatrixXi eye(size_t n) {
    MatrixXi A(n, n);
    for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) {
        if (i == j) {
          A.data[i][j] = 1;
        } else {
          A.data[i][j] = 0;
        }
      }
    }
    return A;
  }

  static MatrixXi kron(const MatrixXi &m, const MatrixXi &n) {
    size_t a = m.row;
    size_t b = m.col;
    size_t c = n.row;
    size_t d = n.col;
    MatrixXi t(a * c, b * d);
    for (size_t i = 0; i < a * c; i++) {
      for (size_t j = 0; j < b * d; j++) {
        t.data[i][j] = m.data[i / c][j / d] * n.data[i % c][j % d];
      }
    }
    return t;
  }

  ~MatrixXi() { data.clear(); }

  int &operator()(size_t i, size_t j) { return data[i][j]; }
  const int &operator()(size_t i, size_t j) const { return data[i][j]; }
  size_t rows() const { return row; }
  size_t cols() const { return col; }

  void resize(size_t nr, size_t nc) {
    data.resize(nr);
    for (size_t i = 0; i < nr; i++) {
      data[i].resize(nc);
    }
    col = nc;
    row = nr;
  }

 private:
  size_t row;
  size_t col;
  vector<vector<int>> data;
};

#endif
