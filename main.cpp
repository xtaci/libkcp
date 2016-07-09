#include <unistd.h>
#include <sys/time.h>
#include "sess.h"
#include <iostream>

int main() {
    struct timeval time;
    gettimeofday(&time,NULL);
    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    UDPSession *sess = UDPSession::Dial("127.0.0.1", 8888);
    int count;
    for (;;) {
        sess->Update();
        sleep(1);
        count++;
        if (count > 10) {
            break;
        };
        std::cout << count<< std::endl;
    }
    sess->Close();
}