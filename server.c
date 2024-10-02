#include "common.h"
#include <pthread.h>

#define MAX_CLIENTS 10

typedef struct
{
    int sockfd;
    char username[USERNAME_MAX_LEN];
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Broadcast message to all clients except the sender
void broadcast_message(Message *msg, int exclude_sockfd)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].sockfd != 0 && clients[i].sockfd != exclude_sockfd)
        {
            if (send_message(clients[i].sockfd, msg) == -1)
            {
                perror("send_message");
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

// Check if username is already taken
int is_username_taken(const char *username)
{
    int taken = 0;
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].sockfd != 0 && strcmp(clients[i].username, username) == 0)
        {
            taken = 1;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return taken;
}

// Handle client communication
void *handle_client(void *arg)
{
    int sockfd = *(int *)arg;
    free(arg);

    Message msg;
    int res = receive_message(sockfd, &msg);
    if (res == -1 || msg.type != MSG_TYPE_TEXT)
    {
        close(sockfd);
        pthread_exit(NULL);
    }

    if (is_username_taken(msg.username))
    {
        // Username is taken
        Message response;
        response.type = MSG_TYPE_EXIT;
        strcpy(response.data, "Username already taken.");
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }

    // Add client to clients list
    pthread_mutex_lock(&clients_mutex);

    int i;
    for (i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].sockfd == 0)
        {
            clients[i].sockfd = sockfd;
            strncpy(clients[i].username, msg.username, USERNAME_MAX_LEN);
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    if (i == MAX_CLIENTS)
    {
        // Max clients reached
        Message response;
        response.type = MSG_TYPE_EXIT;
        strcpy(response.data, "Server full.");
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("%s has connected.\n", msg.username);

    // Main loop
    while (1)
    {
        res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            printf("%s has disconnected.\n", msg.username);
            break;
        }

        // Broadcast message to other clients
        broadcast_message(&msg, sockfd);
    }

    // Remove client from clients list
    pthread_mutex_lock(&clients_mutex);
    clients[i].sockfd = 0;
    clients[i].username[0] = '\0';
    pthread_mutex_unlock(&clients_mutex);

    close(sockfd);
    pthread_exit(NULL);
}

int main()
{
    int server_sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(server_addr.sin_zero), 0, 8);

    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(server_sockfd, MAX_CLIENTS) == -1)
    {
        perror("listen");
        exit(1);
    }

    printf("Server listening on port %d\n", PORT);

    while (1)
    {
        sin_size = sizeof(struct sockaddr_in);
        new_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_sockfd == -1)
        {
            perror("accept");
            continue;
        }

        printf("Got connection from %s\n", inet_ntoa(client_addr.sin_addr));

        // Create a thread to handle client
        pthread_t tid;
        int *pclient = malloc(sizeof(int));
        *pclient = new_sockfd;

        if (pthread_create(&tid, NULL, handle_client, pclient) != 0)
        {
            perror("pthread_create");
            continue;
        }

        pthread_detach(tid);
    }

    return 0;
}
