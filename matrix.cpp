//
// Created by 理 傅 on 2016/12/30.
//

#include <cstdlib>
#include <cstring>
#include "matrix.h"


matrix *
matrix::newMatrix(int rows, int cols) {
    if (rows <= 0 || cols <= 0) {
        return nullptr;
    }

    matrix *m = new(matrix);
    m->data = (uint8_t **) malloc(sizeof(uintptr_t) * rows);
    for (int i = 0; i < rows; i++) {
        m->data[i] = (uint8_t *) malloc(sizeof(uint8_t) * cols);
        memset(m->data[i], 0, sizeof(uint8_t) * cols);
    }
    return m;
}

matrix *matrix::identityMatrix(int size) {
    matrix *m = newMatrix(size, size);
    if (m == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < size; i++) {
        m->data[i][i] = 1;
    }
    return m;
}

