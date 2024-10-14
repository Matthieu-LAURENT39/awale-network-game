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
    // A text message, displays the username
    MSG_TYPE_TEXT,
    // Exits the client
    MSG_TYPE_EXIT,
    // Server message, doesn't display the username
    MSG_TYPE_SERVER,
    // message for info of party, doesn't display anything
    MSG_TYPE_INFO,
    // message for private message
    MSG_TYPE_MP,
    // message for game
    MSG_TYPE_GAME
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
