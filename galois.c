//
// Created by 理 傅 on 2016/12/30.
//

#include "galois.h"

char galAdd(char a, char b) {
    return a ^ b;
}

char galSub(char a, char b) {
    return a ^ b;
}

char galMultiply(char a, char b) {
    return mulTable[a][b];
}

char galDivide(char a, char b) {
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

char galExp(char a, char n) {
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