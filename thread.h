#include <pthread.h>

void create_new_thread(int newsockfd);

void check_thread_completion(void);

void* socket_thread(void *thread_param);