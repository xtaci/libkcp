//
//  CRC32.h
//  libkcp
//
//  Created by 孔祥波 on 04/05/2017.
//  Copyright © 2017 Kong XiangBo. All rights reserved.
//

#ifndef CRC32_h
#define CRC32_h

#include <stdio.h>
#include <stdlib.h>
int32_t crc32(uint8_t *bytes, size_t len);

unsigned int cal_crc(unsigned char *ptr, unsigned char len);
uint32_t
crc32_kr(uint32_t crc, const uint8_t *buf, size_t size);
#endif /* CRC32_h */
