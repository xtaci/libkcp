//
// Created by 理 傅 on 16/7/11.
//

#ifndef KCP_IO_H
#define KCP_IO_H

#include <sys/types.h>

class ReadWriter {
public:
    // Read into buf with size sz, return > 0 for number of bytes read, < 0 for error
    virtual ssize_t Read(char *buf, size_t sz) = 0;

    // Write into buf with size sz, return > 0 for number of bytes written, < 0 for error
    virtual ssize_t Write(const char *buf, size_t sz) = 0;

    // PeekSize returns readable size.
    virtual ssize_t PeekSize() = 0;
};

#endif //KCP_IO_H
