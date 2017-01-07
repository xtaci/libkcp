//
// Created by 理 傅 on 2016/12/30.
//

#include "galois.h"
#include "matrix.h"

matrix
matrix::newMatrix(size_t rows, size_t cols) {
    matrix m;
    m.rows = rows;
    m.cols = cols;
    m.data.resize(rows, nullptr);
    for (auto i = 0; i < rows; i++) {
        m.data[i] = std::make_shared<std::vector<byte>>(cols);
    }
    return m;
}

matrix
matrix::identityMatrix(int size) {
    matrix m = matrix::newMatrix(size, size);
    for (auto i = 0; i < size; i++) {
        m.at(i, i) = 1;
    }
    return m;
}


matrix
matrix::Multiply(matrix &right) {
    if (cols != right.rows) {
        return matrix{};
    }

    matrix result = matrix::newMatrix(rows, right.cols);

    for (auto r = 0; r < result.rows; r++) {
        for (auto c = 0; c < result.cols; c++) {
            byte value = 0;
            for (auto i = 0; i < this->cols; i++) {
                value ^= galMultiply(at(r, i), right.at(i, c));
            }
            result.at(r, c) = value;
        }
    }
    return result;
}

matrix
matrix::Augment(matrix &right) {
    matrix result = matrix::newMatrix(this->rows, this->cols + right.cols);

    for (auto r = 0; r < this->rows; r++) {
        for (auto c = 0; c < this->cols; c++) {
            result.at(r, c) = at(r, c);
        }
        auto cols = this->cols;
        for (auto c = 0; c < right.cols; c++) {
            result.at(r, cols + c) = right.at(r, c);
        }
    }
    return result;
}

matrix
matrix::SubMatrix(int rmin, int cmin, int rmax, int cmax) {
    matrix result = matrix::newMatrix(rmax - rmin, cmax - cmin);
    for (auto r = rmin; r < rmax; r++) {
        for (auto c = cmin; c < cmax; c++) {
            result.at(r - rmin, c - cmin) = at(r, c);
        }
    }
    return result;
}

// SwapRows Exchanges two rows in the matrix.
int
matrix::SwapRows(int r1, int r2) {
    if (r1 < 0 || rows <= r1 || r2 < 0 || rows <= r2) {
        return -1;
    }

    std::swap(data[r1], data[r2]);
    return 0;
}

bool
matrix::IsSquare() {
    return this->rows == this->cols;
}

matrix
matrix::Invert() {
    if (!IsSquare()) {
        return matrix{};
    }
    auto work = matrix::identityMatrix(rows);
    work = matrix::Augment(work);

    auto ret = work.gaussianElimination();
    if (ret != 0) {
        return matrix{};
    }

    return work.SubMatrix(0, rows, rows, rows * 2);
}

int
matrix::gaussianElimination() {
    auto rows = this->rows;
    auto columns = this->cols;
    // Clear out the part below the main diagonal and scale the main
    // diagonal to be 1.
    for (auto r = 0; r < rows; r++) {
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
            for (auto c = 0; c < columns; c++) {
                at(r, c) = galMultiply(at(r, c), scale);
            }
        }

        // Make everything below the 1 be a 0 by subtracting
        // a multiple of it.  (Subtraction and addition are
        // both exclusive or in the Galois field.)
        for (int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
            if (at(rowBelow, r) != 0) {
                byte scale = at(rowBelow, r);
                for (auto c = 0; c < columns; c++) {
                    at(rowBelow, c) ^= galMultiply(scale, at(r, c));
                }
            }
        }
    }

    // Now clear the part above the main diagonal.
    for (auto d = 0; d < rows; d++) {
        for (auto rowAbove = 0; rowAbove < d; rowAbove++) {
            if (at(rowAbove, d) != 0) {
                byte scale = at(rowAbove, d);
                for (auto c = 0; c < columns; c++) {
                    at(rowAbove, c) ^= galMultiply(scale, at(d, c));
                }
            }
        }
    }
    return 0;
}

matrix
matrix::vandermonde(size_t rows, size_t cols) {
    matrix result = matrix::newMatrix(rows, cols);
    for (auto r = 0; r < rows; r++) {
        for (auto c = 0; c < cols; c++) {
            result.at(r, c) = galExp(byte(r), byte(c));
        }
    }
    return result;
}
