#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

void initialise_socket_param(char* port, struct addrinfo** servinfo);

int create_and_bind_socket(struct addrinfo* servinfo);

void accept_and_create_thread_session(int sockfd);