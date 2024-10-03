#include "common.h"
#include "color.h"
#include <pthread.h>

// Thread to handle incoming messages
void *receive_handler(void *arg)
{
    int sockfd = *(int *)arg;
    Message msg;
    while (1)
    {
        int res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            printf("Disconnected from server.\n");
            close(sockfd);
            exit(1);
        }
        else if (msg.type == MSG_TYPE_TEXT)
        {
            printf("%s%s%s:%s %s%s%s\n", CHAT_USERNAME_STYLE, STYLE_BOLD, msg.username, COLOR_RESET, CHAT_TEXT_STYLE, msg.data, COLOR_RESET);
        }
        else if (msg.type == MSG_TYPE_SERVER)
        {
            printf("%s\n", msg.data);
        }
    }
    return NULL;
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    char username[USERNAME_MAX_LEN];

    printf("Enter username: ");
    fgets(username, USERNAME_MAX_LEN, stdin);
    username[strcspn(username, "\n")] = '\0'; // Remove newline

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // Replace with server IP if needed
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(server_addr.sin_zero), 0, 8);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    // Send username to server
    Message msg;
    msg.type = MSG_TYPE_TEXT;
    strncpy(msg.username, username, USERNAME_MAX_LEN);
    strcpy(msg.data, "has joined the chat.");
    if (send_message(sockfd, &msg) == -1)
    {
        perror("send_message");
        exit(1);
    }
    char output[BUFFER_SIZE];
    colorize("Connection successful", SERVER_SUCCESS_STYLE, STYLE_BOLD, output);
    printf("%s\n", output);

    // Start thread to receive messages
    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_handler, &sockfd) != 0)
    {
        perror("pthread_create");
        exit(1);
    }

    // Main loop to send messages
    while (1)
    {
        char input[BUFFER_SIZE];
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "/exit") == 0)
        {
            msg.type = MSG_TYPE_EXIT;
            send_message(sockfd, &msg);
            break;
        }
        else
        {
            msg.type = MSG_TYPE_TEXT;
            strcpy(msg.data, input);
            if (send_message(sockfd, &msg) == -1)
            {
                perror("send_message");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}
