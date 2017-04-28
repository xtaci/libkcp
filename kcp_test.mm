#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include "sess.h"

IUINT32 iclock();

int kcptest(const char *addr,int port) {
    struct timeval time;
    gettimeofday(&time, NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    UDPSession *sess = UDPSession::DialWithOptions(addr, port, 2,2);
    sess->NoDelay(1, 20, 2, 1);
    sess->WndSize(128, 128);
    sess->SetMtu(1400);
    sess->SetStreamMode(true);
    sess->SetDSCP(46);

    assert(sess != nullptr);
    ssize_t nsent = 0;
    ssize_t nrecv = 0;
    char *buf = (char *) malloc(128);

    for (int i = 0; i < 10; i++) {
        sprintf(buf, "message:%d", i);
        auto sz = strlen(buf);
        sess->Write(buf, sz);
        sess->Update(iclock());
        memset(buf, 0, 128);
        ssize_t n = 0;
        do {
            n = sess->Read(buf, 128);
            if (n > 0) { printf("%s\n", buf); }
            usleep(33000);
            sess->Update(iclock());
        } while(n==0);
    }

    UDPSession::Destroy(sess);
    return  0 ;
}




