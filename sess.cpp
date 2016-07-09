//
// Created by 理 傅 on 16/7/9.
//

#include "sess.h"
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <stdlib.h>


Session * Session::DialKCP(char * network, char * address){
    int sockfd = socket(PF_INET, SOCK_DGRAM ,0);
    if(sockfd == -1){
        return 0;
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);



    Session *sess = new(Session);
    sess->m_sockfd = sockfd;
    sess->kcp = ikcp_create(rand(), sess);

    return sess;
}



