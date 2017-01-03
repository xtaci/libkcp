//
// Created by 理 傅 on 2017/1/1.
//

#include <cstdlib>
#include <vector>
#include "reedsolomon.h"
#include "galois_noasm.h"

ReedSolomon *
ReedSolomon::New(int dataShards, int parityShards) {
    if (dataShards<=0 || parityShards <=0) {
        return nullptr;
    }

    if (dataShards + parityShards > 255) {
        return nullptr;
    }

    ReedSolomon * r = new ReedSolomon;
    r->DataShards = dataShards;
    r->ParityShards = parityShards;
    r->Shards = dataShards + parityShards;

    // Start with a Vandermonde matrix.  This matrix would work,
    // in theory, but doesn't have the property that the data
    // shards are unchanged after encoding.
    matrix * vm  = matrix::vandermonde(r->Shards, r->DataShards);
    if (vm == nullptr){
        return nullptr;
    }

    // Multiply by the inverse of the top square of the matrix.
    // This will make the top square be the identity matrix, but
    // preserve the property that any square subset of rows  is
    // invertible.
    auto top = vm->SubMatrix(0,0, dataShards,dataShards);
    top = top->Invert();
    r->m = vm->Multiply(top);

    // Inverted matrices are cached in a tree keyed by the indices
    // of the invalid rows of the data to reconstruct.
    // The inversion root node will have the identity matrix as
    // its inversion matrix because it implies there are no errors
    // with the original data.
    r->tree = inversionTree::newInversionTree(dataShards, parityShards);

    r->parity = (byte**)malloc(sizeof(byte*) * parityShards);
    for (int i=0;i<parityShards;i++) {
        r->parity[i] = r->m->m[dataShards+i];
    }
    return r;
}

int
ReedSolomon::Encode(byte **shards, int count, size_t shardSize) {
    if (count != this->DataShards) {
        return -1;
    }

    // Get the slice of output buffers.
    byte **output = &shards[DataShards];

    // Do the coding.
    this->codeSomeShards(parity, shards, output, ParityShards, shardSize);
    return 0;
};


void
ReedSolomon::codeSomeShards(byte **matrixRows, byte ** inputs, byte **outputs, int outputCount, size_t byteCount) {
    for (int c = 0; c < DataShards; c++) {
        auto in = inputs[c];
        for (int iRow = 0; iRow < outputCount; iRow++) {
            if (c == 0) {
                galMulSlice(matrixRows[iRow][c], in, outputs[iRow], byteCount);
            } else {
                galMulSliceXor(matrixRows[iRow][c], in, outputs[iRow], byteCount);
            }
        }
    }
}

int
ReedSolomon::Reconstruct(byte ** shards, size_t numShards, size_t shardSize) {
    // Quick check: are all of the shards present?  If so, there's
    // nothing to do.
    int numberPresent = 0;
    for (int i = 0; i < Shards; i++) {
        if (shards[i] != nullptr) {
            numberPresent++;
        }
    }

    if (numberPresent == Shards) {
        // Cool.  All of the shards data data.  We don't
        // need to do anything.
        return 0;
    }

    // More complete sanity check
    if (numberPresent < DataShards) {
        return -1;
    }

    // Pull out an array holding just the shards that
    // correspond to the rows of the submatrix.  These shards
    // will be the input to the decoding process that re-creates
    // the missing data shards.
    //
    // Also, create an array of indices of the valid rows we do have
    // and the invalid rows we don't have up until we have enough valid rows.
    byte ** subShards = (byte **) malloc(sizeof(byte *) * DataShards);
    memset(subShards, 0, sizeof(byte *) * DataShards);

    std::vector<int> validIndices(DataShards, 0);
    std::vector<int> invalidIndices;
    int subMatrixRow = 0;

    for (int matrixRow = 0; matrixRow < Shards && subMatrixRow < DataShards; matrixRow++) {
        if (shards[matrixRow] != nullptr) {
            subShards[subMatrixRow] = shards[matrixRow];
            validIndices[subMatrixRow] = matrixRow;
            subMatrixRow++;
        } else {
            invalidIndices.push_back(matrixRow);
        }
    }

    // Attempt to get the cached inverted matrix out of the tree
    // based on the indices of the invalid rows.
    auto dataDecodeMatrix = tree->GetInvertedMatrix(invalidIndices);

    // If the inverted matrix isn't cached in the tree yet we must
    // construct it ourselves and insert it into the tree for the
    // future.  In this way the inversion tree is lazily loaded.
    if (dataDecodeMatrix == nullptr) {
        // Pull out the rows of the matrix that correspond to the
        // shards that we have and build a square matrix.  This
        // matrix could be used to generate the shards that we have
        // from the original data.
        auto subMatrix = matrix::newMatrix(DataShards, DataShards);
        for (int subMatrixRow = 0; subMatrixRow < validIndices.size(); subMatrixRow++) {
            for (int c = 0; c < DataShards; c++) {
                subMatrix->m[subMatrixRow][c] = m->m[validIndices[subMatrixRow]][c];
            };
        }

        // Invert the matrix, so we can go from the encoded shards
        // back to the original data.  Then pull out the row that
        // generates the shard that we want to decode.  Note that
        // since this matrix maps back to the original data, it can
        // be used to create a data shard, but not a parity shard.
        auto dataDecodeMatrix = subMatrix->Invert();
        if (dataDecodeMatrix == nullptr) {
            return -2;
        }

        // Cache the inverted matrix in the tree for future use keyed on the
        // indices of the invalid rows.
        if (int ret = tree->InsertInvertedMatrix(invalidIndices, std::shared_ptr<matrix>(dataDecodeMatrix), Shards) && ret != 0) {
            return -3;
        }
    }

    // Re-create any data shards that were missing.
    //
    // The input to the coding is all of the shards we actually
    // have, and the output is the missing data shards.  The computation
    // is done using the special decode matrix we just built.
    byte ** outputs = (byte **) malloc(sizeof(byte *) * ParityShards);
    memset(outputs, 0, sizeof(uint8_t *) * ParityShards);

    byte ** matrixRows = (byte **) malloc(sizeof(byte *) * ParityShards);
    memset(matrixRows, 0, sizeof(uint8_t *) * ParityShards);

    int outputCount = 0;

    for (int iShard = 0;iShard < DataShards; iShard++) {
        if (shards[iShard] == nullptr) {
            shards[iShard] = (byte *) malloc(sizeof(byte) * shardSize);
            outputs[outputCount] = shards[iShard];
            matrixRows[outputCount] = dataDecodeMatrix->m[iShard];
            outputCount++;
        }
    }
    codeSomeShards(matrixRows, subShards, outputs+outputCount, outputCount, shardSize);

    // Now that we have all of the data shards intact, we can
    // compute any of the parity that is missing.
    //
    // The input to the coding is ALL of the data shards, including
    // any that we just calculated.  The output is whichever of the
    // data shards were missing.
    outputCount = 0;
    for (int iShard = 0;iShard < DataShards; iShard++) {
        if (shards[iShard] == nullptr) {
            shards[iShard] = (byte *) malloc(sizeof(byte) * shardSize);
            outputs[outputCount] = shards[iShard];
            matrixRows[outputCount] = parity[iShard-DataShards];
            outputCount++;
        }
    }
    codeSomeShards(matrixRows, shards + DataShards, outputs+outputCount, outputCount, shardSize);

    return 0;
}



