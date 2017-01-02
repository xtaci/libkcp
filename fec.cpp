//
// Created by 理 傅 on 2017/1/2.
//

#include <err.h>
#include <cstdlib>
#include <sys/time.h>
#include "fec.h"
#include "ikcp.h"

/* decode 32 bits unsigned int (lsb) */
static inline void decode32u(const char *p, uint32_t *l)
{
#if IWORDS_BIG_ENDIAN
    *l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else
    *l = *(uint32_t*)p;
#endif
}

/* decode 16 bits unsigned int (lsb) */
static inline void decode16u(const char *p, uint16_t *w)
{
#if IWORDS_BIG_ENDIAN
    *w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
    *w = *(const unsigned short*)p;
#endif
}

FEC *
FEC::newFEC(int rxlimit, int dataShards, int parityShards) {
    if (dataShards <= 0 || parityShards <= 0) {
        return nullptr;
    }

    if (rxlimit < dataShards+parityShards) {
        return nullptr;
    }

    auto fec = new(FEC);
    fec->rxlimit = rxlimit;
    fec->dataShards = dataShards;
    fec->parityShards = parityShards;
    fec->shardSize = dataShards + parityShards;
    fec->paws = (0xffffffff/uint32_t(fec->shardSize) - 1) * uint32_t(fec->shardSize);
    auto enc = ReedSolomon::New(dataShards, parityShards);
    fec->enc = enc;
    fec->shards = (char **) malloc(sizeof(char *) * fec->shardSize);
    return fec;
}

fecPacket
FEC::decode(char* data, size_t sz) {
    fecPacket pkt;
    decode32u(data, &pkt.seqid);
    decode16u(data+4, &pkt.flag);
    struct timeval time;
    gettimeofday(&time, NULL);
    pkt.ts = uint32_t(time.tv_sec * 1000 + time.tv_usec/1000);
    pkt.data = (char*)malloc(sizeof(char*) * sz);
    memcpy(pkt.data, data, sz);
    return pkt;
}

