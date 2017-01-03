//
// Created by 理 傅 on 2017/1/2.
//

#include <err.h>
#include <cstdlib>
#include <sys/time.h>
#include "fec.h"
#include "ikcp.h"
#include "sess.h"

/* encode 16 bits unsigned int (lsb) */
static inline char *encode16u(char *p, unsigned short w)
{
#if IWORDS_BIG_ENDIAN
    *(unsigned char*)(p + 0) = (w & 255);
	*(unsigned char*)(p + 1) = (w >> 8);
#else
    *(unsigned short*)(p) = w;
#endif
    p += 2;
    return p;
}

/* decode 16 bits unsigned int (lsb) */
static inline char *decode16u(char *p, unsigned short *w)
{
#if IWORDS_BIG_ENDIAN
    *w = *(const unsigned char*)(p + 1);
	*w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
    *w = *(const unsigned short*)p;
#endif
    p += 2;
    return p;
}

/* encode 32 bits unsigned int (lsb) */
static inline char *encode32u(char *p, IUINT32 l)
{
#if IWORDS_BIG_ENDIAN
    *(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
    *(IUINT32*)p = l;
#endif
    p += 4;
    return p;
}

/* decode 32 bits unsigned int (lsb) */
static inline char *decode32u(char *p, IUINT32 *l)
{
#if IWORDS_BIG_ENDIAN
    *l = *(const unsigned char*)(p + 3);
	*l = *(const unsigned char*)(p + 2) + (*l << 8);
	*l = *(const unsigned char*)(p + 1) + (*l << 8);
	*l = *(const unsigned char*)(p + 0) + (*l << 8);
#else
    *l = *(const IUINT32*)p;
#endif
    p += 4;
    return p;
}

fecPacket::~fecPacket() {
    if (this->data != nullptr) {
        free(this->data);
    }
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

fecPacket *
FEC::decode(char* data, size_t sz) {
    fecPacket *pkt = new(fecPacket);
    data = decode32u(data, &pkt->seqid);
    data =decode16u(data, &pkt->flag);
    struct timeval time;
    gettimeofday(&time, NULL);
    pkt->ts = uint32_t(time.tv_sec * 1000 + time.tv_usec/1000);
    pkt->data = (char*)malloc(sizeof(char*) * sz);
    memcpy(pkt->data, data, sz);
    return pkt;
}

void
FEC::markData(char *data) {
    data = encode32u(data,this->next);
    data = encode16u(data,FEC::typeData);
    this->next++;
}

void
FEC::markFEC(char *data) {
    data = encode32u(data,this->next);
    data = encode16u(data,FEC::typeFEC);
    this->next++;
    if (this->next >= this->paws) { // paws would only occurs in markFEC
        this->next = 0;
    }
}

int
FEC::input(fecPacket *pkt, uint8_t ** shards, size_t *numShards, int *sz) {
    uint32_t now = currentMs();
    if (now-lastCheck >= FEC::fecExpire) {
        for (auto it = rx.begin();it !=rx.end();) {
            if (now - (*it)->ts > FEC::fecExpire) {
                it = rx.erase(it);
            } else {
                it++;
            }
        }
        lastCheck = now;
    }

    // insertion
    int n = this->rx.size() -1;
    int insertIdx = 0;
    for (int i=n;i>=0;i--) {
        if (pkt->seqid == rx[i]->seqid) {
            return 0;
        } else if (pkt->seqid > rx[i]->seqid) {
            insertIdx = i + 1;
            break;
        }
    }
    // insert into ordered rx queue
    rx.insert(rx.begin()+insertIdx, pkt);

    // shard range for current packet
    int shardBegin = pkt->seqid - pkt->seqid%shardSize;
    int shardEnd = shardBegin + shardSize - 1;

    // max search range in ordered queue for current shard
    int searchBegin = insertIdx - pkt->seqid%shardSize;
    if (searchBegin < 0) {
        searchBegin = 0;
    }

    int searchEnd = searchBegin + shardSize - 1;
    if (searchEnd >= rx.size()) {
        searchEnd = rx.size()-1;
    }
    return 0;
}


