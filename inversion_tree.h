//
// Created by 理 傅 on 2017/1/1.
//

#ifndef KCP_INVERSION_TREE_H
#define KCP_INVERSION_TREE_H

#include <cstddef>
#include "matrix.h"

class inversionNode {
public:
    matrix * m;
    inversionNode **children;
    matrix* getInvertedMatrix(int *invalidIndices, size_t sz, int parent);
    void insertInvertedMatrix(int *invalidIndices, size_t sz, matrix * matrix, int shards, int parent);
};

class inversionTree {
public:
    inversionTree(const inversionTree &) = delete;

    inversionTree &operator=(const inversionTree &) = delete;

    inversionNode * root;

    // newInversionTree initializes a tree for storing inverted matrices.
    // Note that the root node is the identity matrix as it implies
    // there were no errors with the original data.
    static inversionTree * newInversionTree(int dataShards, int parityShards);


    // GetInvertedMatrix returns the cached inverted matrix or nil if it
    // is not found in the tree keyed on the indices of invalid rows.
    matrix * GetInvertedMatrix(int *invalidIndices, size_t sz);
private:
    inversionTree() = default;
    ~inversionTree() = default;
};



#endif //KCP_INVERSION_TREE_H
