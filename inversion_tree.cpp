//
// Created by 理 傅 on 2017/1/1.
//

#include "inversion_tree.h"

inversionTree *inversionTree::newInversionTree(int dataShards, int parityShards) {
    matrix *identity = matrix::identityMatrix(dataShards);
    inversionNode root{
            .m = std::shared_ptr<matrix>(identity),
            .children = new inversionNode*[dataShards + parityShards](),
    };
    return new inversionTree(root);
}


std::shared_ptr<matrix>
inversionTree::GetInvertedMatrix(std::vector<int> & invalidIndices) {
    if (invalidIndices.size() == 0) {
        return root.m;
    }

    return root.getInvertedMatrix(invalidIndices, 0);
}

int
inversionTree::InsertInvertedMatrix(std::vector<int> & invalidIndices, std::shared_ptr<matrix> matrix, int shards) {
    // If no invalid indices were given then we are done because the
    // root node is already set with the identity matrix.
    if (invalidIndices.size() == 0) {
        return -1;
    }

    if(!matrix->IsSquare()) {
        return -2;
    }

    // Recursively create nodes for the inverted matrix in the tree until
    // we reach the node to insert the matrix to.  We start by passing in
    // 0 as the parent index as we start at the root of the tree.
    root.insertInvertedMatrix(invalidIndices, matrix, shards, 0);

    return 0;
}


std::shared_ptr<matrix>
inversionNode::getInvertedMatrix(std::vector<int> & invalidIndices, int parent) {
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
    if (invalidIndices.size() > 1) {
        // Search recursively on the child node by passing in the invalid indices
        // with the first index popped off the front.  Also the parent index to
        // pass down is the first index plus one.
        std::vector<int> subvector(invalidIndices.begin() +1,invalidIndices.end());
        return getInvertedMatrix(subvector,firstIndex+1);
    }

    // If there aren't any more invalid indices to search, we've found our
    // node.  Return it, however keep in mind that the matrix could still be
    // nil because intermediary nodes in the tree are created sometimes with
    // their inversion matrices uninitialized.
    return m;
}

void
inversionNode::insertInvertedMatrix(std::vector<int> & invalidIndices,std::shared_ptr<matrix> matrix, int shards, int parent){
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
        auto node = new inversionNode {
                .m = nullptr,
                .children = new inversionNode*[shards - firstIndex](),
        };
        children[firstIndex- parent] =node;
    }


    // If there's more than one invalid index left in the list we should
    // keep searching recursively in order to find the node to add our
    // matrix.
    if (invalidIndices.size() > 1) {
        // As above, search recursively on the child node by passing in
        // the invalid indices with the first index popped off the front.
        // Also the total number of shards and parent index are passed down
        // which is equal to the first index plus one.
        std::vector<int> subvector(invalidIndices.begin() +1,invalidIndices.end());
        insertInvertedMatrix(subvector, matrix, shards, firstIndex+1);
    } else {
        this->m = matrix;
    }
}





