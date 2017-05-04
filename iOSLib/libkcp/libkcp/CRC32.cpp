//
//  CRC32.c
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#include "CRC32.h"

int32_t crc32(uint8_t *bytes, size_t len)
{
    uint32_t *table = (uint32_t *)malloc(sizeof(uint32_t) * 256);
    uint32_t crc = 0xffffffff;
    
    uint8_t *ptr = bytes;
    for (uint32_t i=0; i<256; i++) {
        table[i] = i;
        for (int j=0; j<8; j++) {
            if (table[i] & 1) {
                table[i] = (table[i] >>= 1) ^ 0xedb88320;
            } else {
                table[i] >>= 1;
            }
        }
    }
    
    for (int i=0; i< len; i++) {
        crc = (crc >> 8) ^ table[(crc & 0xff) ^ *ptr];
        ptr++;
    }
    crc ^= 0xffffffff;
    
    free(table);
    return crc;
}
