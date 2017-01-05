//
// Created by 理 傅 on 2017/1/4.
//
#include <iostream>
#include "fec.h"


int main() {

    int datashard = 5;
    int parityshard = 3;
    int totalshard = datashard + parityshard;

    FEC fec = FEC::New(20, datashard, parityshard);

    byte arr[] = {0, 0, 0};
    std::vector<row> shards(totalshard);
    for (int i=0;i<datashard;i++) {
        for (int j = 0;j<3;j++) {
            arr[j] = byte(rand()%255);
        }
        shards[i] = std::make_shared<std::vector<byte>>(arr, arr +3);
    }

    for (int i=datashard;i<totalshard;i++) {
        shards[i] = std::make_shared<std::vector<byte>>(3);
    }

    std::cout << "shards to encode:" <<  std::endl;
    for (int i =0;i<shards.size();i++) {
        for (auto b : *shards[i]) {
            std::cout << int(b)  << " ";
        }
        std::cout << std::endl;
    }

    auto ret = fec.Encode(shards);
    std::cout << "encoded:" << std::endl;
    for (int i =0;i<shards.size();i++) {
        for (auto b : *shards[i]) {
            std::cout << int(b)  << " ";
        }
        std::cout << std::endl;
    }

    // remove first 3 element
    for (int i =0;i<shards.size();i++) {
        if (i < 3) {
            continue;
        }

        fecPacket pkt;
        pkt.data = shards[i];
        pkt.seqid = i;
        if (i < 5) {
            pkt.flag = typeData;
        } else {
            pkt.flag = typeFEC;
        }
        auto recovered = fec.Input(pkt);

        if (recovered.size() > 0) {
            std::cout << "recovered" << std::endl;
            for (int i =0;i<recovered.size();i++) {
                for (auto b : *recovered[i]) {
                    std::cout << int(b)  << " ";
                }
                std::cout << std::endl;
            }
        }
    }
}

