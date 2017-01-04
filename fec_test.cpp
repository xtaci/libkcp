//
// Created by 理 傅 on 2017/1/4.
//
#include <iostream>
#include "fec.h"

int main() {

    FEC fec = FEC::newFEC(20, 5, 3);

    int arr[] = {0};

    std::vector<row> shards(5);
    for (int i=0;i<5;i++) {
        arr[0] = i;
        shards[i] = std::make_shared<std::vector<byte>>(arr, arr +1);
    }

    for (int i =0;i<shards.size();i++) {
        for (auto b : *shards[i]) {
            std::cout << int(b) << std::endl;
        }
    }

    auto parity = fec.calcECC(shards);
    std::cout << "empty:" << parity.size() << std::endl;
    for (int i =0;i<parity.size();i++) {
        for (auto b : *parity[i]) {
            std::cout << int(b) << std::endl;
        }
    }
}