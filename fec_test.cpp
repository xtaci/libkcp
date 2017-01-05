//
// Created by 理 傅 on 2017/1/4.
//
#include <iostream>
#include "fec.h"

int main() {

    int datashard = 5;
    int parityshard = 3;
    int totalshard = datashard + parityshard;

    FEC fec = FEC::newFEC(20, datashard, parityshard);

    int arr[] = {0};
    std::vector<row> shards(totalshard);
    for (int i=0;i<datashard;i++) {
        arr[0] = i;
        shards[i] = std::make_shared<std::vector<byte>>(arr, arr +1);
    }

    for (int i=datashard;i<totalshard;i++) {
        shards[i] = std::make_shared<std::vector<byte>>(1);
    }

    for (int i =0;i<shards.size();i++) {
        for (auto b : *shards[i]) {
            std::cout << int(b) << std::endl;
        }
    }

    auto ret =  fec.calcECC(shards);
    std::cout << "ret:" << ret << std::endl;
    for (int i =0;i<shards.size();i++) {
        for (auto b : *shards[i]) {
            std::cout << int(b) << std::endl;
        }
    }
}