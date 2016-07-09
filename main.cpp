#include <unistd.h>
#include <sys/time.h>
#include "sess.h"
#include <iostream>

int main() {
    struct timeval time;
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    UDPSession *sess = UDPSession::Dial("127.0.0.1", 9999);
    int count;
    char *buf = (char *)malloc(128);
    for (;;) {
        sprintf(buf, "%d", count);
        sess->Write(buf,strlen(buf));
        sess->Update();
        memset(buf,0,128);
        ssize_t n = sess->Read(buf,128);
        printf("%s\n",buf);

        sleep(1);
        count++;
        if (count > 10) {
            break;
        };
    }
    sess->Close();
}