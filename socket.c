#include "socket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "thread.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

void initialise_socket_param(char* port, struct addrinfo** servinfo) {
    int status;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port, &hints, servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }
}

int create_and_bind_socket(struct addrinfo* servinfo) {
    int sockfd;
    struct addrinfo* p;
    int yes = 1;
    for(p = servinfo; p != NULL;p = p->ai_next) {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    /*servinfo is no longer required*/
    freeaddrinfo(servinfo);

    if(p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }
    return sockfd;
}

void accept_and_create_thread_session(int sockfd) {
    struct sockaddr_storage clientaddr;
    socklen_t clientsize = sizeof(struct sockaddr_storage);
    int newsockfd = accept(sockfd, (struct sockaddr*)&clientaddr, &clientsize);
    if(newsockfd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    /*Spawn a new thread to handle multiple new sessions/connections*/
    create_new_thread(newsockfd);
}

