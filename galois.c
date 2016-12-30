//
// Created by 理 傅 on 2016/12/30.
//

#include "galois.h"

uint8_t galAdd(uint8_t a, uint8_t b) {
    return a ^ b;
}

uint8_t galSub(uint8_t a, uint8_t b) {
    return a ^ b;
}

uint8_t galMultiply(uint8_t a, uint8_t b) {
    return mulTable[a][b];
}

uint8_t galDivide(uint8_t a, uint8_t b) {
    if (a == 0) {
        return 0;
    }

    /*
    if b == 0 {
        panic("Argument 'divisor' is 0")
    }
     */
    int logA = logTable[a];
    int logB = logTable[b];
    int logResult = logA - logB;
    if (logResult < 0) {
        logResult += 255;
    }
    return expTable[logResult];
}

uint8_t galExp(uint8_t a, uint8_t n) {
    if (n == 0) {
        return 1;
    }
    if (a == 0) {
        return 0;
    }

    int logA = logTable[a];
    int logResult = logA * n;
    while (logResult >= 255) {
        logResult -= 255;
    }
    return expTable[logResult];
}