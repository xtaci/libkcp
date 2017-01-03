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
    fec->shardBuffer = (byte **)malloc(sizeof(byte*) * fec->shardSize);
    for (int i=0;i<fec->shardSize;i++) {
        fec->shardBuffer[i] = (byte*)malloc(sizeof(byte) * FEC::mtuLimit);
    }

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
    pkt->sz = sz;
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
FEC::input(fecPacket *pkt, std::vector<byte *> *recovered) {
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
    rx.insert(rx.begin()+insertIdx, std::shared_ptr<fecPacket>(pkt));

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

    if (searchEnd > searchBegin && searchEnd-searchBegin+1 >= dataShards) {
        int numshard = 0;
        int numDataShard = 0;
        int first = -1;
        int maxlen = 0;

        std::vector<int> shards(shardSize);
        std::vector<bool> shardsflag(shardSize, false);

        for (int i = searchBegin; i <= searchEnd; i++) {
            auto seqid = rx[i]->seqid;
            if (seqid > shardEnd) {
                break;
            } else if (seqid >= shardBegin) {
                shards[seqid%shardSize] = i;
                shardsflag[seqid%shardSize] = true;
                numshard++;
                if (rx[i]->flag == typeData) {
                    numDataShard++;
                }
                if (numshard == 1) {
                    first = i;
                }
                if (rx[i]->sz > maxlen) {
                    maxlen = rx[i]->sz;
                }
            }
        }

        if (numDataShard == dataShards) { // no lost
            rx.erase(rx.begin()+first, rx.begin() + first+numshard);
        } else if (numshard >= dataShards) { // recoverable
            zerobuffer();
            for (int k=0;k<shardSize;k++){
                if (!shardsflag[k]) {
                    memcpy(shardBuffer[k], rx[shards[k]]->data, rx[shards[k]]->sz);
                }
            }

            if (int ret = enc->Reconstruct(shardBuffer, shardSize, FEC::mtuLimit) && ret== 0 ){
                for (int k=0;k<dataShards;k++){
                    if (!shardsflag[k]) {
                        recovered->push_back(shardBuffer[k]);
                    }
                }
            }
            rx.erase(rx.begin()+first, rx.begin() + first+numshard);
        }
    }

    // keep rxlimit
    if (rx.size() > rxlimit) {
        rx.erase(rx.begin());
    }

    return 0;
}

int
FEC::calcECC(byte ** data, int offset, int count, int maxlen) {
    if (count != shardSize) {
        return -1;
    }

    byte ** shards = (byte**)malloc(sizeof(byte*) * shardSize);
    memset(shards, 0, sizeof(byte*) * shardSize);
    for (int i=0;i<count;i++) {
        shards[i] = data[i] + offset;
    }

    this->enc->Encode(shards, count, maxlen - offset);
    return 0;
}


void
FEC::zerobuffer() {
    for (int i=0;i<shardSize;i++) {
        memset(shardBuffer[i], 0, FEC::mtuLimit);
    }
}


