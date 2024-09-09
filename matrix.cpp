//
// Created by 理 傅 on 2016/12/30.
//

#include <stdexcept>
#include "galois.h"
#include "matrix.h"

int Matrix::GaussianElimination() {
  auto rows = this->rows;
  auto columns = this->cols;
  // Clear out the part below the main diagonal and scale the main
  // diagonal to be 1.
  for (int r = 0; r < rows; r++) {
    // If the element on the diagonal is 0, find a row below
    // that has a non-zero and swap them.
    if (at(r, r) == 0) {
      for (int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
        if (at(rowBelow, r) != 0) {
          this->SwapRows(r, rowBelow);
          break;
        }
      }
    }

    // If we couldn't find one, the matrix is singular.
    if (at(r, r) == 0) {
      return -1;
    }

    // Scale to 1.
    if (at(r, r) != 1) {
      byte scale = galDivide(1, at(r, r));
      for (int c = 0; c < columns; c++) {
        at(r, c) = galMultiply(at(r, c), scale);
      }
    }

    // Make everything below the 1 be a 0 by subtracting
    // a multiple of it.  (Subtraction and addition are
    // both exclusive or in the Galois field.)
    for (int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
      if (at(rowBelow, r) != 0) {
        byte scale = at(rowBelow, r);
        for (int c = 0; c < columns; c++) {
          at(rowBelow, c) ^= galMultiply(scale, at(r, c));
        }
      }
    }
  }

  // Now clear the part above the main diagonal.
  for (int d = 0; d < rows; d++) {
    for (int rowAbove = 0; rowAbove < d; rowAbove++) {
      if (at(rowAbove, d) != 0) {
        byte scale = at(rowAbove, d);
        for (int c = 0; c < columns; c++) {
          at(rowAbove, c) ^= galMultiply(scale, at(d, c));
        }
      }
    }
  }
  return 0;
}

int Matrix::SwapRows(int r1, int r2) {
  if (r1 < 0 || rows <= r1 || r2 < 0 || rows <= r2) {
    return -1;
  }

  std::swap(data[r1], data[r2]);
  return 0;
}

MatrixPtr Multiply(const MatrixPtr& left, const MatrixPtr& right) {
  if (left->cols != right->rows) {
    return nullptr;
  }

  auto result = std::make_shared<Matrix>(left->rows, right->cols);

  for (int r = 0; r < result->rows; r++) {
    for (int c = 0; c < result->cols; c++) {
      byte value = 0;
      for (int i = 0; i < left->cols; i++) {
        value ^= galMultiply(left->at(r, i), right->at(i, c));
      }
      result->at(r, c) = value;
    }
  }
  return result;
}

MatrixPtr Augment(const MatrixPtr& left, const MatrixPtr& right) {
  auto result = std::make_shared<Matrix>(left->rows, left->cols + right->cols);

  for (int r = 0; r < left->rows; r++) {
    for (int c = 0; c < left->cols; c++) {
      result->at(r, c) = left->at(r, c);
    }
    auto cols = left->cols;
    for (int c = 0; c < right->cols; c++) {
      result->at(r, cols + c) = right->at(r, c);
    }
  }
  return result;
}

MatrixPtr SubMatrix(const MatrixPtr& mp, int rmin, int cmin, int rmax, int cmax) {
  auto result = std::make_shared<Matrix>(rmax - rmin, cmax - cmin);
  for (int r = rmin; r < rmax; r++) {
    for (int c = cmin; c < cmax; c++) {
      result->at(r - rmin, c - cmin) = mp->at(r, c);
    }
  }
  return result;
}

MatrixPtr Invert(const MatrixPtr& mp) {
  if (!mp->IsSquare()) {
    return nullptr;
  }
  std::shared_ptr<Matrix> work = std::make_shared<IdentityMatrix>(mp->rows);
  work = Augment(mp, work);

  auto ret = work->GaussianElimination();
  if (ret != 0) {
    return nullptr;
  }

  return SubMatrix(work, 0, mp->rows, mp->rows, mp->rows * 2);
}