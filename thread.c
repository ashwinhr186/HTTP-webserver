#include "thread.h"
#include <stdbool.h>
#include "includes/queue.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MAXLINE  1000000  /* max text line length */

typedef struct thread {
    pthread_t thread_id;
    int newsockfd;
    bool thread_completion;
}thread_t;

typedef struct llnode {
    thread_t thread_param;
    SLIST_ENTRY(llnode) next;
}llnode_t;

/*Initialize ll head node*/
SLIST_HEAD(slisthead, llnode) head;

void create_new_thread(int newsockfd) {
    llnode_t *newllnode = (llnode_t*)malloc(sizeof(llnode_t));
    newllnode->thread_param.newsockfd = newsockfd;
    newllnode->thread_param.thread_completion = false;
    pthread_create(&newllnode->thread_param.thread_id, NULL, socket_thread, &newllnode->thread_param);
    SLIST_INSERT_HEAD(&head, newllnode, next);
    check_thread_completion();
}

void check_thread_completion(void) {
    llnode_t *node = NULL;
    SLIST_FOREACH(node, &head, next) {
        if(node->thread_param.thread_completion == true) {
            int ret = pthread_join(node->thread_param.thread_id, NULL);
            if(ret != 0) {
                continue;
            }
            SLIST_REMOVE(&head, node, llnode, next);
            free(node);           
        }
    }
}

void* socket_thread(void *thread_param) {
    thread_t *thread_func_arg = (thread_t*)thread_param;
    size_t request_length = 0;
    int fd = 0;
    char tbuf[MAXLINE]; 
    char rbuf[MAXLINE];
    char* temp_buf = NULL;
    int content_length = 0;
    char * content_type = NULL;
    char httpmsg[2*MAXLINE];
    memset(tbuf, 0, MAXLINE);
    memset(rbuf, 0, MAXLINE); 
    memset(httpmsg, 0, 2*MAXLINE);
    char document_root[MAXLINE] = "www";
    char header[MAXLINE];

    request_length = recv(thread_func_arg->newsockfd, rbuf, MAXLINE, 0);
    if(request_length == -1) {
        perror("read");
        return NULL;
    }
    if (request_length == 0) {
        printf("client closed connection\n");
        close(thread_func_arg->newsockfd);
        thread_func_arg->thread_completion = true;
        return NULL;
    }
   // printf("newsockfd: %d\n", thread_func_arg->newsockfd);
    printf("server received the following request:\n%s\n",rbuf);
    temp_buf = (char*)malloc(strlen(rbuf));
    strcpy(temp_buf, rbuf);

    /*Tokenize the input received into Request Method, Request URI and Request Version*/
    char *token = strtok(temp_buf, " ");
    char *req_method = token;
    token = strtok(NULL, " ");
    char *req_URI = token;
    token = strtok(NULL, " ");
    char *req_version = token;
    char *nextline = strchr(req_version, '\n');
    if(nextline != NULL) {
        *nextline = '\0';
    }

    char* format = strrchr(req_URI, '.');
    if(format != NULL) {
        if (strcmp(format, ".ico") == 0) {
            content_type = "image/x-icon";
        }
        if(strcmp(format, ".html") == 0) {
            content_type = "text/html";
        }
        else if(strcmp(format, ".txt") == 0) {
            content_type = "text/plain";
        }
        else if(strcmp(format, ".jpg") == 0) {
            content_type = "image/jpg";
        }
        else if(strcmp(format, ".gif") == 0) {
            content_type = "image/gif";
        }
        else if(strcmp(format, ".png") == 0) {
            content_type = "image/png";
        }
        else if(strcmp(format, ".css") == 0) {
            content_type = "text/css";
        }
        else if(strcmp(format, ".js") == 0) {
            content_type = "application/javascript";
        }
    }
    else {
        content_type = "text/html";
    }

    /*Check Request Method*/
    if(strcmp(req_method, "GET") == 0) {
        if(strcmp(req_URI, "/") == 0) {
            fd = open("www/index.html", O_RDONLY);
            struct stat st;
            fstat(fd, &st);
            int size = st.st_size;

            sprintf(header, "%s 200 Document Follows\r\n Content-Type: %s\r\nContent-Length: %d\r\n\r\n", req_version, content_type, size);
            send(thread_func_arg->newsockfd, header, strlen(header), 0);
            content_length = read(fd, httpmsg, MAXLINE);
            if (content_length == -1) {
                perror("read");
                return NULL;
            }
            send(thread_func_arg->newsockfd, httpmsg, content_length, 0);
        }
        else {
            strcat(document_root, req_URI);
            fd = open(document_root, O_RDONLY);
            if(fd == -1) {
                sprintf(httpmsg, "%s 404 Not Found\r\n", req_version);
            }
            else {
                //Get file size for content-length
                struct stat st;
                fstat(fd, &st);
                int size = st.st_size;

                sprintf(header, "%s 200 Document Follows\r\n Content-Type: %s\r\nContent-Length: %d\r\n\r\n", req_version, content_type, size);
                send(thread_func_arg->newsockfd, header, strlen(header), 0);
                content_length = read(fd, httpmsg, 2*MAXLINE);
                printf("content_length: %d\n", content_length);
                if (content_length == -1) {
                     perror("read");
                     sprintf(httpmsg, "%s 500 Internal Server Error\r\n", req_version);
                }
                send(thread_func_arg->newsockfd, httpmsg, content_length, 0);
            }
        }
    }

    else if (strcmp(req_method, "POST") == 0) {
        strcat(document_root, req_URI);
        fd = open(document_root, O_RDONLY);
        if(fd == -1) {
            sprintf(httpmsg, "%s 404 Not Found\r\n", req_version);
        }
        else {
            //Get file size for content-length
            struct stat st;
            fstat(fd, &st);
            size_t size = st.st_size;

            //Extract POST DATA from the request
            char *post_data = strstr(rbuf, "\r\n\r\n");
            post_data = post_data + 4;
            size_t post_data_len = strlen(post_data);
            size = size + post_data_len + 40;
           
           printf("req_version: %s\n", req_version);
            sprintf(header, "HTTP/1.1 200 OK\r\nContent-type: %s\r\nContent-size: %zu\r\n<html><body><pre><h1>%s</h1></pre>", content_type, size, post_data);
            send(thread_func_arg->newsockfd, header, strlen(header), 0);
            content_length = read(fd, httpmsg, 2*MAXLINE); 
            if (content_length == -1) {
                perror("read");
                sprintf(httpmsg, "%s 500 Internal Server Error\r\n", req_version);
            }
            send(thread_func_arg->newsockfd, httpmsg, content_length, 0);
        }
    }

    //printf("server returning a http message with the following content.\n%s\n", httpmsg);
    //send(thread_func_arg->newsockfd, httpmsg,strlen(httpmsg), 0);
    close(thread_func_arg->newsockfd);
    close(fd);
    thread_func_arg->thread_completion = true;
    return NULL;
}