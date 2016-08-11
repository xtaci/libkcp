#ifndef KCP_IO_H
#define KCP_IO_H

#include <sys/types.h>

// ReadWriter has non-blocking semantics on stream.
class ReadWriter {
public:
    // Read into buf with size sz, return > 0 for number of bytes read, < 0 for error
    virtual ssize_t Read(char *buf, size_t sz) = 0;

    // Write into buf with size sz, return > 0 for number of bytes written, < 0 for error
    virtual ssize_t Write(const char *buf, size_t sz) = 0;
};

#endif //KCP_IO_H
