//
// Created by 理 傅 on 2016/12/30.
//

#ifndef KCP_MATRIX_H
#define KCP_MATRIX_H

#include <vector>
#include <memory>
#include "galois.h"

// newMatrix returns a matrix of zeros.
class Matrix {
 public:
  Matrix(int r, int c) : rows(r), cols(c) {
    data = new byte*[r];
    for (int i = 0; i < r; i++) data[i] = new byte[c]{0};
  }

  ~Matrix() {
    for (int i = 0; i < rows; i++) delete[] data[i];
    delete[] data;
    data = nullptr;
  }

  inline byte& at(int row, int col) { return data[row][col]; }

  inline bool IsSquare() { return this->rows == this->cols; }

  // SwapRows Exchanges two rows in the matrix.
  int SwapRows(int r1, int r2);

  //  Gaussian elimination (also known as row reduction)
  int GaussianElimination();

  int rows{0}, cols{0};

  byte** data;
};

// IdentityMatrix returns an identity matrix of the given empty.
class IdentityMatrix : public Matrix {
 public:
  IdentityMatrix(int size) : Matrix(size, size) {
    for (int i = 0; i < size; i++) {
      at(i, i) = 1;
    }
  }
};

// Create a Vandermonde matrix, which is guaranteed to have the
// property that any subset of rows that forms a square matrix
// is invertible.
class VandermondeMatrix : public Matrix {
 public:
  VandermondeMatrix(int rows, int cols) : Matrix(rows, cols) {
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        at(r, c) = galExp(byte(r), byte(c));
      }
    }
  }
};

using MatrixPtr = std::shared_ptr<Matrix>;

// Multiply multiplies this matrix (the one on the left) by another
// matrix (the one on the right) and returns a new matrix with the result.
MatrixPtr Multiply(const MatrixPtr& left, const MatrixPtr& right);

// Augment returns the concatenation of this matrix and the matrix on the right.
MatrixPtr Augment(const MatrixPtr& left, const MatrixPtr& right);

// Returns a part of this matrix. Data is copied.
MatrixPtr SubMatrix(const MatrixPtr& mp, int rmin, int cmin, int rmax, int cmax);

// Invert returns the inverse of this matrix.
// Returns ErrSingular when the matrix is singular and doesn't have an inverse.
// The matrix must be square, otherwise ErrNotSquare is returned.
MatrixPtr Invert(const MatrixPtr& mp);

#endif //KCP_MATRIX_H
