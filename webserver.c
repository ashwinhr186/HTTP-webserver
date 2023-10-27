#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "socket.h"
#include <fcntl.h>
#include <sys/socket.h>

#define BACKLOG 10

int global_signal_flag = 0;

/*****************************************************************
 *                  Server's main entry point
 ****************************************************************/

int main(int argc, char* argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Invalid number of arguments!\nUsage: %s <port number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /*Register signal for SIGINT and SIGTERM for graceful termination*/
    //signal(SIGINT, exit_gracefully);
    //signal(SIGTERM, exit_gracefully);

    char* port = argv[1];
    struct addrinfo* servinfo;
    
    initialise_socket_param(port, &servinfo);

    int sockfd = create_and_bind_socket(servinfo);
    if(sockfd == -1) {
        perror("socket/bind");
        exit(EXIT_FAILURE);
    }

    int listen_status = listen(sockfd, BACKLOG);
    if(listen_status == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while(1) {
        accept_and_create_thread_session(sockfd);
    }
}
