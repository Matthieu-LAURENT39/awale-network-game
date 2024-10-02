#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define USERNAME_MAX_LEN 32

// Message types
typedef enum
{
    MSG_TYPE_TEXT,
    MSG_TYPE_EXIT
} MessageType;

typedef struct
{
    MessageType type;
    char username[USERNAME_MAX_LEN];
    char data[BUFFER_SIZE];
} Message;

// Function prototypes
int send_message(int sockfd, Message *msg);
int receive_message(int sockfd, Message *msg);

#endif // COMMON_H
