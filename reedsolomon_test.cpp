#include <cassert>
#include <vector>
#include <sys/time.h>
#include "reedsolomon.h"
#include "fec.h"

static std::vector<row_type> buildShardSet(int dataShards, int parityShards, const std::vector<std::vector<byte>> &payload) {
    int total = dataShards + parityShards;
    std::vector<row_type> shards(total);
    for (int i = 0; i < dataShards; i++) {
        shards[i] = std::make_shared<std::vector<byte>>(payload[i]);
    }
    for (int i = dataShards; i < total; i++) {
        shards[i] = std::make_shared<std::vector<byte>>(payload[0].size(), 0);
    }
    return shards;
}

static void testConstructorMatchesFactory() {
    const int dataShards = 4;
    const int parityShards = 2;
    const int shardLen = 6;

    std::vector<std::vector<byte>> payload(dataShards, std::vector<byte>(shardLen));
    for (int i = 0; i < dataShards; i++) {
        for (int j = 0; j < shardLen; j++) {
            payload[i][j] = byte((i + 1) * (j + 3));
        }
    }

    auto shardsFromCtor = buildShardSet(dataShards, parityShards, payload);
    auto shardsFromFactory = buildShardSet(dataShards, parityShards, payload);

    ReedSolomon directCtor(dataShards, parityShards);
    auto factoryCtor = ReedSolomon::New(dataShards, parityShards);

    directCtor.Encode(shardsFromCtor);
    factoryCtor.Encode(shardsFromFactory);

    for (int i = dataShards; i < dataShards + parityShards; i++) {
        assert(shardsFromCtor[i]->size() == shardsFromFactory[i]->size());
        for (size_t j = 0; j < shardsFromCtor[i]->size(); j++) {
            assert((*shardsFromCtor[i])[j] == (*shardsFromFactory[i])[j]);
        }
    }
}

static void testZeroLengthShardsEncodeReconstruct() {
    const int dataShards = 3;
    const int parityShards = 2;
    const int total = dataShards + parityShards;

    auto encoder = ReedSolomon::New(dataShards, parityShards);
    std::vector<row_type> shards(total);
    for (int i = 0; i < total; i++) {
        shards[i] = std::make_shared<std::vector<byte>>();
    }

    encoder.Encode(shards);
    for (int i = dataShards; i < total; i++) {
        assert(shards[i]->empty());
    }

    shards[0] = nullptr;
    encoder.Reconstruct(shards);
    assert(shards[0] != nullptr);
    assert(shards[0]->empty());
}

static void testFecAcceptsFirstPacket() {
    const int dataShards = 3;
    const int parityShards = 2;
    FEC fec = FEC::New(32, dataShards, parityShards);

    fecPacket pkt;
    pkt.seqid = 0;
    pkt.flag = typeData;
    pkt.data = std::make_shared<std::vector<byte>>(4, byte(42));
    struct timeval tv;
    gettimeofday(&tv, NULL);
    pkt.ts = uint32_t(tv.tv_sec * 1000 + tv.tv_usec / 1000);

    auto recovered = fec.Input(pkt);
    assert(recovered.empty());
}

int main() {
    testConstructorMatchesFactory();
    testZeroLengthShardsEncodeReconstruct();
    testFecAcceptsFirstPacket();
    return 0;
}
