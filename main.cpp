#include <unistd.h>
#include <sys/time.h>
#include "sess.h"
#include <iostream>

IUINT32 iclock();

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    UDPSession *sess = UDPSession::DialIPv6("::1", 9999);
    assert(sess != nullptr);
    int count;
    char *buf = (char *) malloc(128);
    for (; ;) {
        sprintf(buf, "%d", count);
        sess->Write(buf, strlen(buf));
        sess->Update(iclock());
        memset(buf, 0, 128);
        ssize_t n = sess->Read(buf, 128);
        printf("%s\n", buf);

        sleep(1);
        count++;
        if (count > 10) {
            break;
        };
    }
    UDPSession::Destroy(sess);
}


void
itimeofday(long *sec, long *usec) {
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
}

IUINT64 iclock64(void) {
    long s, u;
    IUINT64 value;
    itimeofday(&s, &u);
    value = ((IUINT64) s) * 1000 + (u / 1000);
    return value;
}

IUINT32 iclock() {
    return (IUINT32) (iclock64() & 0xfffffffful);
}


