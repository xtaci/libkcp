//
// Created by 理 傅 on 2017/1/1.
//

#include <cstdlib>
#include <vector>
#include "reedsolomon.h"
#include "galois_noasm.h"

ReedSolomon::ReedSolomon(int dataShards, int parityShards):
        m_dataShards(dataShards),
        m_parityShards(parityShards),
        m_totalShards(dataShards + parityShards) {
    tree = inversionTree::newInversionTree(dataShards, parityShards);
}

ReedSolomon
ReedSolomon::New(int dataShards, int parityShards) {
    if (dataShards<=0 || parityShards <=0) {
        throw std::invalid_argument("invalid arguments");
    }

    if (dataShards + parityShards > 255) {
        throw std::invalid_argument("invalid arguments");
    }

    ReedSolomon r(dataShards, parityShards);

    // Start with a Vandermonde matrix.  This matrix would work,
    // in theory, but doesn't have the property that the data
    // shards are unchanged after encoding.
    matrix vm  = matrix::vandermonde(r.m_totalShards, r.m_dataShards);

    // Multiply by the inverse of the top square of the matrix.
    // This will make the top square be the identity matrix, but
    // preserve the property that any square subset of rows  is
    // invertible.
    auto top = vm.SubMatrix(0,0, dataShards,dataShards);
    top = top.Invert();
    r.m = vm.Multiply(top);

    // Inverted matrices are cached in a tree keyed by the indices
    // of the invalid rows of the data to reconstruct.
    // The inversion m_root node will have the identity matrix as
    // its inversion matrix because it implies there are no errors
    // with the original data.
    r.tree = inversionTree::newInversionTree(dataShards, parityShards);

    r.parity = std::vector<row>(parityShards);
    for (int i=0;i<parityShards;i++) {
        r.parity[i] = r.m.data[dataShards+i];
    }
    return r;
}

int
ReedSolomon::Encode(std::vector<row> &shards) {
    if (shards.size() != m_totalShards) {
        return -1;
    }

    // Get the slice of output buffers.
    std::vector<row> output(shards.begin() + m_dataShards, shards.end());

    // Do the coding.
    std::vector<row> input(shards.begin(), shards.begin() + this->m_dataShards);
    codeSomeShards(parity, input , output, m_parityShards);
    return 0;
};


void
ReedSolomon::codeSomeShards(std::vector<row> &matrixRows, std::vector<row> & inputs, std::vector<row> & outputs, int outputCount) {
    for (int c = 0; c < m_dataShards; c++) {
        auto in = inputs[c];
        for (int iRow = 0; iRow < outputCount; iRow++) {
            if (c == 0) {
                galMulSlice((*matrixRows[iRow])[c], in, outputs[iRow]);
            } else {
                galMulSliceXor((*matrixRows[iRow])[c], in, outputs[iRow]);
            }
        }
    }
}

int
ReedSolomon::Reconstruct(std::vector<row> &shards) {
    if (shards.size() != m_totalShards) {
        return -1;
    }

    auto shardSize = this->shardSize(shards);

    // Quick check: are all of the shards present?  If so, there's
    // nothing to do.
    int numberPresent = 0;
    for (int i = 0; i < m_totalShards; i++) {
        if (shards[i]->size() != 0) {
            numberPresent++;
        }
    }

    if (numberPresent == m_totalShards) {
        // Cool.  All of the shards data data.  We don't
        // need to do anything.
        return -2;
    }

    // More complete sanity check
    if (numberPresent < m_dataShards) {
        return -3;
    }

    // Pull out an array holding just the shards that
    // correspond to the rows of the submatrix.  These shards
    // will be the input to the decoding process that re-creates
    // the missing data shards.
    //
    // Also, create an array of indices of the valid rows we do have
    // and the invalid rows we don't have up until we have enough valid rows.
    std::vector<row> subShards(m_dataShards);
    std::vector<int> validIndices(m_dataShards, 0);
    std::vector<int> invalidIndices;
    int subMatrixRow = 0;

    for (int matrixRow = 0; matrixRow < m_totalShards && subMatrixRow < m_dataShards; matrixRow++) {
        if (shards[matrixRow]->size() != 0) {
            subShards[subMatrixRow] = shards[matrixRow];
            validIndices[subMatrixRow] = matrixRow;
            subMatrixRow++;
        } else {
            invalidIndices.push_back(matrixRow);
        }
    }

    // Attempt to get the cached inverted matrix out of the tree
    // based on the indices of the invalid rows.
    auto dataDecodeMatrix = tree.GetInvertedMatrix(invalidIndices);

    // If the inverted matrix isn't cached in the tree yet we must
    // construct it ourselves and insert it into the tree for the
    // future.  In this way the inversion tree is lazily loaded.
    if (dataDecodeMatrix.empty()) {
        // Pull out the rows of the matrix that correspond to the
        // shards that we have and build a square matrix.  This
        // matrix could be used to generate the shards that we have
        // from the original data.
        auto subMatrix = matrix::newMatrix(m_dataShards, m_dataShards);
        for (int subMatrixRow = 0; subMatrixRow < validIndices.size(); subMatrixRow++) {
            for (int c = 0; c < m_dataShards; c++) {
                subMatrix.at(subMatrixRow,c) = m.at(validIndices[subMatrixRow], c);
            };
        }

        // Invert the matrix, so we can go from the encoded shards
        // back to the original data.  Then pull out the row that
        // generates the shard that we want to decode.  Note that
        // since this matrix maps back to the original data, it can
        // be used to create a data shard, but not a parity shard.
        auto dataDecodeMatrix = subMatrix.Invert();
        if (dataDecodeMatrix.empty()) {
            return -2;
        }

        // Cache the inverted matrix in the tree for future use keyed on the
        // indices of the invalid rows.
        if (int ret = tree.InsertInvertedMatrix(invalidIndices, dataDecodeMatrix, m_totalShards) && ret != 0) {
            return -3;
        }
    }

    // Re-create any data shards that were missing.
    //
    // The input to the coding is all of the shards we actually
    // have, and the output is the missing data shards.  The computation
    // is done using the special decode matrix we just built.
    std::vector<row> outputs;
    std::vector<row> matrixRows(m_parityShards);
    int outputCount = 0;

    for (int iShard = 0;iShard < m_dataShards; iShard++) {
        if (shards[iShard]->size() == 0) {
            shards[iShard] = std::make_shared<std::vector<byte>>(shardSize);
            outputs.push_back(shards[iShard]);
            matrixRows[outputCount] = dataDecodeMatrix.data[iShard];
            outputCount++;
        }
    }

    codeSomeShards(matrixRows, subShards, outputs, outputCount);

    // Now that we have all of the data shards intact, we can
    // compute any of the parity that is missing.
    //
    // The input to the coding is ALL of the data shards, including
    // any that we just calculated.  The output is whichever of the
    // data shards were missing.
    outputCount = 0;
    outputs.clear();
    for (int iShard = 0;iShard < m_dataShards; iShard++) {
        if (shards[iShard]->size() == 0) {
            shards[iShard] = std::make_shared<std::vector<byte>>(shardSize);
            outputs.push_back(shards[iShard]);
            matrixRows[outputCount] = parity[iShard-m_dataShards];
            outputCount++;
        }
    }

    std::vector<row> ds(shards.begin(), shards.begin() + m_dataShards);
    codeSomeShards(matrixRows, shards, outputs, outputCount);

    return 0;
}


int ReedSolomon::shardSize(std::vector<row> & shards)  {
    for (int i =0;i<shards.size();i++) {
        if (shards[i]->size() != 0){
            return shards[i]->size();
        }
    }

    return 0;
}
