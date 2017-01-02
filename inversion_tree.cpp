//
// Created by 理 傅 on 2017/1/1.
//

#include "inversion_tree.h"
#include "matrix.h"
#include <cstdlib>
#include <cstring>

inversionTree *inversionTree::newInversionTree(int dataShards, int parityShards) {
    matrix *identity = matrix::identityMatrix(dataShards);
    inversionNode *root = new inversionNode;
    root->m = identity;
    root->children = (inversionNode **) malloc(sizeof(inversionNode *) * (dataShards + parityShards));
    memset(root->children, 0, sizeof(inversionNode *) * (dataShards + parityShards));
    inversionTree *ret = new inversionTree;
    ret->root = root;
    return ret;
}

matrix *
inversionTree::GetInvertedMatrix(int *invalidIndices, size_t sz) {
    if (sz == 0) {
        return this->root->m;
    }

    return root->getInvertedMatrix(invalidIndices, sz, 0);
}

int
inversionTree::InsertInvertedMatrix(int *invalidIndices, size_t sz, matrix * matrix, int shards) {
    if (sz == 0) {
        return -1;
    }

    if(!matrix->IsSquare()) {
        return -2;
    }

    // Recursively create nodes for the inverted matrix in the tree until
    // we reach the node to insert the matrix to.  We start by passing in
    // 0 as the parent index as we start at the root of the tree.
    root->insertInvertedMatrix(invalidIndices, sz, matrix, shards, 0);
}


matrix *inversionNode::getInvertedMatrix(int *invalidIndices, size_t sz, int parent) {
    // Get the child node to search next from the list of children.  The
    // list of children starts relative to the parent index passed in
    // because the indices of invalid rows is sorted (by default).  As we
    // search recursively, the first invalid index gets popped off the list,
    // so when searching through the list of children, use that first invalid
    // index to find the child node.
    int firstIndex = invalidIndices[0];
    inversionNode *node = children[firstIndex-parent];

    // If the child node doesn't exist in the list yet, fail fast by
    // returning, so we can construct and insert the proper inverted matrix.
    if (node == nullptr) {
        return nullptr;
    }

    // If there's more than one invalid index left in the list we should
    // keep searching recursively.
    if (sz > 1) {
        // Search recursively on the child node by passing in the invalid indices
        // with the first index popped off the front.  Also the parent index to
        // pass down is the first index plus one.
        return getInvertedMatrix(++invalidIndices, --sz, firstIndex+1);
    }

    // If there aren't any more invalid indices to search, we've found our
    // node.  Return it, however keep in mind that the matrix could still be
    // nil because intermediary nodes in the tree are created sometimes with
    // their inversion matrices uninitialized.
    return m;
}

void inversionNode::insertInvertedMatrix(int *invalidIndices, size_t sz, matrix * matrix, int shards, int parent){
    // As above, get the child node to search next from the list of children.
    // The list of children starts relative to the parent index passed in
    // because the indices of invalid rows is sorted (by default).  As we
    // search recursively, the first invalid index gets popped off the list,
    // so when searching through the list of children, use that first invalid
    // index to find the child node.
    int firstIndex = invalidIndices[0];
    inversionNode * node = children[firstIndex-parent];

    // If the child node doesn't exist in the list yet, create a new
    // node because we have the writer lock and add it to the list
    // of children.
    if (node == nullptr) {
        // Make the length of the list of children equal to the number
        // of shards minus the first invalid index because the list of
        // invalid indices is sorted, so only this length of errors
        // are possible in the tree.
        inversionNode * node = new inversionNode;
        node->children = (inversionNode **) malloc(sizeof(inversionNode *) * (shards - firstIndex));
        memset(node->children, 0, sizeof(inversionNode *) * (shards - firstIndex));
        children[firstIndex- parent] = node;
    }


    // If there's more than one invalid index left in the list we should
    // keep searching recursively in order to find the node to add our
    // matrix.
    if (sz > 1) {
        // As above, search recursively on the child node by passing in
        // the invalid indices with the first index popped off the front.
        // Also the total number of shards and parent index are passed down
        // which is equal to the first index plus one.
        insertInvertedMatrix(++invalidIndices, --sz, matrix, shards, firstIndex+1);
    } else {
        this->m = matrix;
    }
}





