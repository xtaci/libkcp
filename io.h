//
// Created by 理 傅 on 16/7/11.
//

#ifndef KCP_IO_H
#define KCP_IO_H

#include <sys/types.h>

class ReadWriter {
public:
    virtual ssize_t Read(char *buf, size_t sz) = 0;
    virtual ssize_t Write(const char *buf, size_t sz) = 0;
};

#endif //KCP_IO_H
