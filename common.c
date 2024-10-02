#include "common.h"

// Send a message to the socket
int send_message(int sockfd, Message *msg)
{
    int total = 0;
    int bytes_left = sizeof(Message);
    int n;
    char *data = (char *)msg;

    while (total < sizeof(Message))
    {
        n = send(sockfd, data + total, bytes_left, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytes_left -= n;
    }

    return n == -1 ? -1 : 0;
}

// Receive a message from the socket
int receive_message(int sockfd, Message *msg)
{
    int total = 0;
    int bytes_left = sizeof(Message);
    int n;
    char *data = (char *)msg;

    while (total < sizeof(Message))
    {
        n = recv(sockfd, data + total, bytes_left, 0);
        if (n <= 0)
        {
            break;
        }
        total += n;
        bytes_left -= n;
    }

    return n <= 0 ? -1 : 0;
}
