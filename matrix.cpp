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
    m->m = (uint8_t **) malloc(sizeof(uintptr_t) * rows);
    for (int i = 0; i < rows; i++) {
        m->m[i] = (uint8_t *) malloc(sizeof(uint8_t) * cols);
        memset(m->m[i], 0, sizeof(uint8_t) * cols);
    }
    return m;
}

matrix *matrix::identityMatrix(int size) {
    matrix *m = matrix::newMatrix(size, size);
    if (m == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < size; i++) {
        m->m[i][i] = 1;
    }
    return m;
}


matrix *matrix::Multiply(matrix *right) {
    if (this->rows != right->cols) {
        return nullptr;
    }

    matrix * result = matrix::newMatrix(this->rows,right->cols);

    for (int r=0;r<result->rows;r++) {
        for (int c=0; c< result->cols;c++) {
            uint8_t value = 0;
            for (int i=0; i<this->cols;i++) {
                value ^= galMultiply(this->m[r][i], right->m[i][c]);
            }
            result->m[r][c] = value;
        }
    }
    return result;
}

matrix *matrix::Augment(matrix *right) {
    if (this->rows != right->cols) {
        return nullptr;
    }

    matrix * result = matrix::newMatrix(this->rows,this->cols + right->cols);

    for (int r=0;r<this->rows;r++) {
        for (int c=0; c<this->cols;c++) {
            result->m[r][c] = this->m[r][c];
        }
        int cols = this->cols;
        for (int c=0; c<right->cols;c++) {
            result->m[r][cols+c] = right->m[r][c];
        }
    }
    return result;
}

matrix *matrix::SubMatrix(int rmin, int cmin, int rmax, int cmax) {
    matrix * result = matrix::newMatrix(rmax-rmin, cmax-cmin);
    for (int r = rmin; r < rmax; r++) {
        for (int c = cmin; c < cmax; c++) {
            result->m[r-rmin][c-cmin] = this->m[r][c];
        }
    }
    return result;
}

bool matrix::IsSquare() {
    return this->rows == this->cols;
}

int matrix::gaussianElimination() {

    return 0;
}




