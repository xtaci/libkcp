//
// Created by 理 傅 on 2017/1/1.
//

#include <vector>
#include <stdexcept>
#include <iostream>
#include "reedsolomon.h"
#include "galois_noasm.h"

ReedSolomon::ReedSolomon(int dataShards, int parityShards) :
        m_dataShards(dataShards),
        m_parityShards(parityShards),
        m_totalShards(dataShards + parityShards) {
    tree = inversionTree::newInversionTree(dataShards, parityShards);
}

ReedSolomon
ReedSolomon::New(int dataShards, int parityShards) {
    if (dataShards <= 0 || parityShards <= 0) {
        throw std::invalid_argument("cannot create Encoder with zero or less data/parity shards");
    }

    if (dataShards + parityShards > 255) {
        throw std::invalid_argument("cannot create Encoder with 255 or more data+parity shards");
    }

    ReedSolomon r(dataShards, parityShards);

    // Start with a Vandermonde matrix.  This matrix would work,
    // in theory, but doesn't have the property that the data
    // shards are unchanged after encoding.
    MatrixPtr vm = std::make_shared<VandermondeMatrix>(r.m_totalShards, r.m_dataShards);

    // Multiply by the inverse of the top square of the matrix.
    // This will make the top square be the identity matrix, but
    // preserve the property that any square subset of rows  is
    // invertible.
    auto top = SubMatrix(vm, 0, 0, dataShards, dataShards);
    top = Invert(top);
    r.m = Multiply(vm, top);

    // Inverted matrices are cached in a tree keyed by the indices
    // of the invalid rows of the data to reconstruct.
    // The inversion m_root node will have the identity matrix as
    // its inversion matrix because it implies there are no errors
    // with the original data.
    r.tree = inversionTree::newInversionTree(dataShards, parityShards);

    r.parity = std::vector<byte *>(parityShards);
    for (int i = 0; i < parityShards; i++) {
      r.parity[i] = r.m->data[dataShards + i];
          // std::make_shared<std::vector<byte>>(r.m->data[dataShards + i], r.m->data[dataShards + i] + r.m->cols);
    }
    return r;
}

void
ReedSolomon::Encode(std::vector<row_type> &shards) {
    if (shards.size() != m_totalShards) {
        throw std::invalid_argument("too few shards given");
    }

    checkShards(shards, false);

    // Get the slice of output buffers.
    static thread_local std::vector<byte*> output(shards.size() - m_dataShards);
    std::fill(output.begin(), output.end(), nullptr);
    for (int i = m_dataShards; i < shards.size(); i++) output[i-m_dataShards] = shards[i]->data();

    // Get the slice of input buffers.
    static thread_local std::vector<byte*> input(m_dataShards);
    std::fill(input.begin(), input.end(), nullptr);
    for (int i = 0; i < m_dataShards; i++) input[i] = shards[i]->data();

    // Do the coding.
    auto indata_size = shards[0]->size();
    codeSomeShards(parity, input, indata_size, output, m_parityShards);
};


void
ReedSolomon::codeSomeShards(std::vector<byte*> &matrixRows, std::vector<byte*> &inputs, int data_size,
                            std::vector<byte*> &outputs, int outputCount) {
    for (int c = 0; c < m_dataShards; c++) {
        auto in = inputs[c];
        for (int r = 0; r < outputCount; r++) {
            if (c == 0) {
                galMulSlice(matrixRows[r][c], in, outputs[r], data_size);
            } else {
                galMulSliceXor(matrixRows[r][c], in, outputs[r], data_size);
            }
        }
    }
}

void
ReedSolomon::Reconstruct(std::vector<row_type> &shards) {
    if (shards.size() != m_totalShards) {
        throw std::invalid_argument("too few shards given");
    }

    // Check arguments
    checkShards(shards,true);

    auto shardSize = this->shardSize(shards);

    // Quick check: are all of the shards present?  If so, there's
    // nothing to do.
    int numberPresent = 0;
    for (int i = 0; i < m_totalShards; i++) {
        if (shards[i] != nullptr) {
            numberPresent++;
        }
    }

    if (numberPresent == m_totalShards) {
        // Cool.  All of the shards data data.  We don't
        // need to do anything.
        return;
    }

    // More complete sanity check
    if (numberPresent < m_dataShards) {
        throw std::invalid_argument("too few shards given");
    }

    // Pull out an array holding just the shards that
    // correspond to the rows of the submatrix.  These shards
    // will be the Input to the decoding process that re-creates
    // the missing data shards.
    //
    // Also, create an array of indices of the valid rows we do have
    // and the invalid rows we don't have up until we have enough valid rows.
    static thread_local std::vector<byte*> subShards(m_dataShards);
    static thread_local std::vector<int> validIndices(m_dataShards, 0);
    static thread_local std::vector<int> invalidIndices;

    // clean
    std::fill(subShards.begin(), subShards.end(), nullptr);
    std::fill(validIndices.begin(), validIndices.end(), 0);
    invalidIndices.clear();
    int subMatrixRow = 0;
    for (int matrixRow = 0; matrixRow < m_totalShards && subMatrixRow < m_dataShards; matrixRow++) {
        if (shards[matrixRow] != nullptr) {
            subShards[subMatrixRow] = shards[matrixRow]->data();
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
    if (dataDecodeMatrix == nullptr) {
        // Pull out the rows of the matrix that correspond to the
        // shards that we have and build a square matrix.  This
        // matrix could be used to generate the shards that we have
        // from the original data.
        auto subMatrix = std::make_shared<Matrix>(m_dataShards, m_dataShards);
        for (subMatrixRow = 0; subMatrixRow < validIndices.size(); subMatrixRow++) {
            for (int c = 0; c < m_dataShards; c++) {
                subMatrix->at(subMatrixRow, c) = m->at(validIndices[subMatrixRow], c);
            };
        }

        // Invert the matrix, so we can go from the encoded shards
        // back to the original data.  Then pull out the row that
        // generates the shard that we want to Decode.  Note that
        // since this matrix maps back to the original data, it can
        // be used to create a data shard, but not a parity shard.
        dataDecodeMatrix = Invert(subMatrix);
        if (dataDecodeMatrix == nullptr) {
            throw std::runtime_error("cannot get matrix invert");
        }

        // Cache the inverted matrix in the tree for future use keyed on the
        // indices of the invalid rows.
        int ret = tree.InsertInvertedMatrix(invalidIndices, dataDecodeMatrix, m_totalShards);
        if (ret != 0) {
            throw std::runtime_error("cannot insert matrix invert");
        }
    }

    // Re-create any data shards that were missing.
    //
    // The Input to the coding is all of the shards we actually
    // have, and the output is the missing data shards.  The computation
    // is done using the special Decode matrix we just built.
    static thread_local std::vector<byte*> outputs(m_parityShards);
    static thread_local std::vector<byte*> matrixRows(m_parityShards);
    // clean
    std::fill(outputs.begin(), outputs.end(), nullptr);
    std::fill(matrixRows.begin(), matrixRows.end(), nullptr);
    int outputCount = 0;

    for (int iShard = 0; iShard < m_dataShards; iShard++) {
        if (shards[iShard] == nullptr) {
            shards[iShard] = std::make_shared<std::vector<byte>>(shardSize);
            outputs[outputCount] = shards[iShard]->data();
            matrixRows[outputCount] = dataDecodeMatrix->data[iShard];
            outputCount++;
        }
    }

    auto indata_size = shards[0]->size();
    codeSomeShards(matrixRows, subShards, indata_size, outputs, outputCount);

    // Now that we have all of the data shards intact, we can
    // compute any of the parity that is missing.
    //
    // The Input to the coding is ALL of the data shards, including
    // any that we just calculated.  The output is whichever of the
    // data shards were missing. 

    outputCount = 0;
    for (int iShard = 0; iShard < m_dataShards; iShard++) subShards[iShard] = shards[iShard]->data();
    for (int iShard = m_dataShards; iShard < m_totalShards; iShard++) {
        if (shards[iShard] == nullptr) {
            shards[iShard] = std::make_shared<std::vector<byte>>(shardSize);
            outputs[outputCount] = shards[iShard]->data();
            matrixRows[outputCount] = parity[iShard - m_dataShards];
            outputCount++;
        }
    }
    codeSomeShards(matrixRows, subShards, indata_size, outputs, outputCount);
}

void
ReedSolomon::checkShards(std::vector<row_type> &shards, bool nilok) {
    auto size = shardSize(shards);
    if (size == 0) {
        throw std::invalid_argument("no shard data");
    }

    for (int i = 0; i < shards.size(); i++) {
        if (shards[i] == nullptr) {
            if (!nilok) {
                throw std::invalid_argument("shard sizes does not match");
            }
        } else if (shards[i]->size() != size) {
            throw std::invalid_argument("shard sizes does not match");
        }
    }
}


int
ReedSolomon::shardSize(std::vector<row_type> &shards) {
    for (int i = 0; i < shards.size(); i++) {
        if (shards[i] != nullptr) {
            return shards[i]->size();
        }
    }

    return 0;
}
