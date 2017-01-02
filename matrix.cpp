//
// Created by 理 傅 on 2016/12/30.
//

#include <cstdlib>
#include <cstring>
#include "galois.h"
#include "matrix.h"

matrix *
matrix::newMatrix(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        return nullptr;
    }

    matrix *m = new(matrix);
    m->rows = rows;
    m->cols = cols;
    m->m = (uint8_t **) malloc(sizeof(uint8_t *) * rows);
    for (int i = 0; i < rows; i++) {
        m->m[i] = (uint8_t *) malloc(sizeof(uint8_t) * cols);
        memset(m->m[i], 0, sizeof(uint8_t) * cols);
    }
    return m;
}

matrix *
matrix::identityMatrix(int size) {
    matrix *m = matrix::newMatrix(size, size);
    for (int i = 0; i < size; i++) {
        m->m[i][i] = 1;
    }
    return m;
}


matrix *
matrix::Multiply(matrix *right) {
    if (this->rows != right->cols) {
        return nullptr;
    }

    matrix *result = matrix::newMatrix(this->rows, right->cols);

    for (int r = 0; r < result->rows; r++) {
        for (int c = 0; c < result->cols; c++) {
            uint8_t value = 0;
            for (int i = 0; i < this->cols; i++) {
                value ^= galMultiply(m[r][i], right->m[i][c]);
            }
            result->m[r][c] = value;
        }
    }
    return result;
}

matrix *
matrix::Augment(matrix *right) {
    if (this->rows != right->cols) {
        return nullptr;
    }

    matrix *result = matrix::newMatrix(this->rows, this->cols + right->cols);

    for (int r = 0; r < this->rows; r++) {
        for (int c = 0; c < this->cols; c++) {
            result->m[r][c] = m[r][c];
        }
        int cols = this->cols;
        for (int c = 0; c < right->cols; c++) {
            result->m[r][cols + c] = right->m[r][c];
        }
    }
    return result;
}

matrix *
matrix::SubMatrix(int rmin, int cmin, int rmax, int cmax) {
    matrix *result = matrix::newMatrix(rmax - rmin, cmax - cmin);
    for (int r = rmin; r < rmax; r++) {
        for (int c = cmin; c < cmax; c++) {
            result->m[r - rmin][c - cmin] = m[r][c];
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

    uint8_t *tmp;
    tmp = m[r2];
    m[r2] = m[r1];
    m[r1] = tmp;
    return 0;
}

bool
matrix::IsSquare() {
    return this->rows == this->cols;
}

matrix *
matrix::Invert() {
    if (!IsSquare()) {
        return nullptr;
    }
    auto work = matrix::identityMatrix(rows);
    work = matrix::Augment(work);

    auto ret = work->gaussianElimination();
    if (ret != 0 ){
        return nullptr;
    }

    return work->SubMatrix(0, rows, rows,rows*2);
}

int
matrix::gaussianElimination() {

    int rows = this->rows;
    int columns = this->cols;
    // Clear out the part below the main diagonal and scale the main
    // diagonal to be 1.
    for (int r = 0; r < rows; r++) {
        // If the element on the diagonal is 0, find a row below
        // that has a non-zero and swap them.
        if (this->m[r][r] == 0) {
            for (int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
                if (this->m[rowBelow][r] != 0) {
                    this->SwapRows(r, rowBelow);
                    break;
                }
            }
        }

        // If we couldn't find one, the matrix is singular.
        if (m[r][r] == 0) {
            return -1;
        }

        // Scale to 1.
        if (m[r][r] != 1) {
            uint8_t scale = galDivide(1, m[r][r]);
            for (int c = 0; c < columns; c++) {
                m[r][c] = galMultiply(m[r][c], scale);
            }
        }

        // Make everything below the 1 be a 0 by subtracting
        // a multiple of it.  (Subtraction and addition are
        // both exclusive or in the Galois field.)
        for (int rowBelow = r + 1; rowBelow < rows; rowBelow++) {
            if (m[rowBelow][r] != 0) {
                uint8_t scale = m[rowBelow][r];
                for (int c = 0; c < columns; c++) {
                    m[rowBelow][c] ^= galMultiply(scale, m[r][c]);
                }
            }
        }
    }

    // Now clear the part above the main diagonal.
    for (int d = 0; d < rows; d++) {
        for (int rowAbove = 0; rowAbove < d; rowAbove++) {
            if (m[rowAbove][d] != 0) {
                uint8_t scale = m[rowAbove][d];
                for (int c = 0; c < columns; c++) {
                    m[rowAbove][c] ^= galMultiply(scale, m[d][c]);
                }
            }
        }
    }
    return 0;
}

matrix *
matrix::vandermonde(int rows, int cols) {
    matrix * result = matrix::newMatrix(rows, cols);
    for (int r = 0;r<rows;r++) {
        for (int c=0;c<cols;c++) {
            result->m[r][c] = galExp(uint8_t(r), uint8_t(c));
        }
    }
    return result;
}
