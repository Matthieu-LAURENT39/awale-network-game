#include "common.h"
#include "game.h"
#include "color.h"
#include <pthread.h>

#define MAX_CLIENTS 10

// Todo: factor out common logic

typedef struct
{
    int sockfd;
    char username[USERNAME_MAX_LEN];
} ClientInfo;

// Structure to represent a challenge
typedef struct Challenge
{
    char challenger[USERNAME_MAX_LEN];
    char challenged[USERNAME_MAX_LEN];
    int game_id;
    struct Challenge *next;
} Challenge;

// Head pointers for active games and challenges
Game *game_list = NULL;
Challenge *challenge_list = NULL;
ClientInfo clients[MAX_CLIENTS];

// Mutexes for thread-safe operations
pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t challenge_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *SERVER_WELCOME_MESSAGE = "Welcome to Matt's Awale server!\nType /help for a list of available commands.";

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

// ========== Game logic ==========
// Add a new challenge
void add_challenge(const char *challenger, const char *challenged, int game_id)
{
    Challenge *new_challenge = (Challenge *)malloc(sizeof(Challenge));
    if (!new_challenge)
    {
        perror("Failed to allocate memory for new challenge");
        return;
    }
    strncpy(new_challenge->challenger, challenger, USERNAME_MAX_LEN - 1);
    new_challenge->challenger[USERNAME_MAX_LEN - 1] = '\0';
    strncpy(new_challenge->challenged, challenged, USERNAME_MAX_LEN - 1);
    new_challenge->challenged[USERNAME_MAX_LEN - 1] = '\0';
    new_challenge->game_id = game_id;
    new_challenge->next = NULL;

    pthread_mutex_lock(&challenge_mutex);
    new_challenge->next = challenge_list;
    challenge_list = new_challenge;
    pthread_mutex_unlock(&challenge_mutex);
}

// Find and remove a challenge
Challenge *find_and_remove_challenge(const char *challenger, const char *challenged)
{
    pthread_mutex_lock(&challenge_mutex);
    Challenge *current = challenge_list;
    Challenge *prev = NULL;
    while (current)
    {
        if (strcmp(current->challenger, challenger) == 0 &&
            strcmp(current->challenged, challenged) == 0)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                challenge_list = current->next;
            }
            pthread_mutex_unlock(&challenge_mutex);
            return current;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&challenge_mutex);
    return NULL;
}

// Send a message to a specific user
void send_to_user(const char *username, Message *msg)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].sockfd != 0 && strcmp(clients[i].username, username) == 0)
        {
            send_message(clients[i].sockfd, msg);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// ========== Main server logic ==========
void handle_command(int sockfd, const char *command, const char *username)
{
    Message response;
    response.type = MSG_TYPE_SERVER;

    if (strcmp(command, "/list") == 0)
    {
        char client_list[BUFFER_SIZE] = "Connected clients:\n";
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i].sockfd != 0)
            {
                strcat(client_list, clients[i].username);
                strcat(client_list, "\n");
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        colorize(client_list, SERVER_SUCCESS_STYLE, NULL, response.data);
        send_message(sockfd, &response);
    }
    else if (strcmp(command, "/help") == 0)
    {
        sprintf(response.data, "%s%sAvailable commands:%s\n"
                               "%s/list - Shows the list of connected clients\n"
                               "/help - Shows this help message\n"
                               "/challenge <username> - Challenge another player to a game\n"
                               "/accept <game_id> - Accept a game challenge\n"
                               "/decline <game_id> - Decline a game challenge\n"
                               "/move <game_id> <hole_number> - Make a move in a specified game\n"
                               "/listgames - List all active games you are part of\n"
                               "/gameinfo <game_id> - Get detailed information about a specific game%s\n",
                SERVER_INFO_STYLE, STYLE_BOLD, COLOR_RESET, SERVER_INFO_STYLE, COLOR_RESET);
        send_message(sockfd, &response);
    }

    // Game commands
    else if (strncmp(command, "/challenge ", 11) == 0)
    {
        char target_username[USERNAME_MAX_LEN];
        sscanf(command + 11, "%s", target_username);

        if (strcmp(target_username, username) == 0)
        {
            colorize("You cannot challenge yourself.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Check if target user exists
        int user_found = 0;
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i].sockfd != 0 && strcmp(clients[i].username, target_username) == 0)
            {
                user_found = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (!user_found)
        {
            colorize("User not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Create a unique game ID (simple increment, could be improved)
        static int next_game_id = 1;
        int game_id = next_game_id++;

        // Add challenge to the list
        add_challenge(username, target_username, game_id);

        // Notify the challenged user
        Message challenge_msg;
        challenge_msg.type = MSG_TYPE_TEXT;
        strcpy(challenge_msg.username, "Server");
        snprintf(challenge_msg.data, BUFFER_SIZE, "You have been challenged by %s. Use /accept %d or /decline %d to respond.", username, game_id, game_id);
        send_to_user(target_username, &challenge_msg);

        // Notify the challenger
        colorize("Challenge sent.", SERVER_SUCCESS_STYLE, NULL, response.data);
        send_message(sockfd, &response);
    }
    else if (strncmp(command, "/accept", 7) == 0)
    {
        int game_id;
        sscanf(command + 8, "%d", &game_id);

        // Find the corresponding challenge
        pthread_mutex_lock(&challenge_mutex);
        Challenge *challenge = NULL;
        Challenge *current = challenge_list;
        Challenge *prev = NULL;
        while (current)
        {
            if (current->game_id == game_id && strcmp(current->challenged, username) == 0)
            {
                challenge = current;
                // Remove from challenge list
                if (prev)
                {
                    prev->next = current->next;
                }
                else
                {
                    challenge_list = current->next;
                }
                break;
            }
            prev = current;
            current = current->next;
        }
        pthread_mutex_unlock(&challenge_mutex);

        if (!challenge)
        {
            colorize("No such challenge found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Create a new game
        Game *new_game = create_game(game_id, challenge->challenger, challenge->challenged);
        if (!new_game)
        {
            colorize("Failed to create game.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            free(challenge);
            return;
        }

        // Add the game to the game list
        pthread_mutex_lock(&game_mutex);
        add_game(&game_list, new_game);
        pthread_mutex_unlock(&game_mutex);

        // Notify both players
        Message game_start_msg;
        game_start_msg.type = MSG_TYPE_TEXT;
        strcpy(game_start_msg.username, "Server");
        char *pos = game_start_msg.data;
        pos += sprintf(pos, "Game %d started between %s%s%s and %s%s%s. It's %s's turn.\n",
                       game_id, STYLE_BOLD, new_game->player_usernames[PLAYER1], COLOR_RESET, STYLE_BOLD,
                       new_game->player_usernames[PLAYER2], COLOR_RESET, new_game->player_usernames[new_game->state.turn]);
        // Todo: probs only need to send to the player whose turn it is
        pos += sprintf(pos, "%s, reply with /move %d <hole_number> to make your move.\n",
                       new_game->player_usernames[new_game->state.turn], game_id);
        send_to_user(new_game->player_usernames[PLAYER1], &game_start_msg);
        send_to_user(new_game->player_usernames[PLAYER2], &game_start_msg);
        free(challenge);

        // Print the initial board state
        pretty_board_state(new_game, response.data);
        send_to_user(new_game->player_usernames[PLAYER1], &response);
        send_to_user(new_game->player_usernames[PLAYER2], &response);
    }
    else if (strncmp(command, "/decline ", 9) == 0)
    {
        int game_id;
        sscanf(command + 9, "%d", &game_id);

        // Find the corresponding challenge
        pthread_mutex_lock(&challenge_mutex);
        Challenge *challenge = NULL;
        Challenge *current = challenge_list;
        Challenge *prev = NULL;
        while (current)
        {
            if (current->game_id == game_id && strcmp(current->challenged, username) == 0)
            {
                challenge = current;
                // Remove from challenge list
                if (prev)
                {
                    prev->next = current->next;
                }
                else
                {
                    challenge_list = current->next;
                }
                break;
            }
            prev = current;
            current = current->next;
        }
        pthread_mutex_unlock(&challenge_mutex);

        if (!challenge)
        {
            colorize("No such challenge found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Notify the challenger
        Message decline_msg;
        decline_msg.type = MSG_TYPE_TEXT;
        strcpy(decline_msg.username, "Server");
        snprintf(decline_msg.data, BUFFER_SIZE, "Your challenge to %s has been declined.", challenge->challenged);
        send_to_user(challenge->challenger, &decline_msg);

        // Notify the decliner
        colorize("Challenge declined.", SERVER_SUCCESS_STYLE, NULL, response.data);
        send_message(sockfd, &response);
        free(challenge);
    }
    else if (strncmp(command, "/move ", 6) == 0)
    {
        int game_id, hole;
        sscanf(command + 6, "%d %d", &game_id, &hole);
        // From here onwards, holes are 0-indexed
        hole--;

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Determine player number
        int player = -1;
        if (strcmp(game->player_usernames[PLAYER1], username) == 0)
        {
            player = PLAYER1;
        }
        else if (strcmp(game->player_usernames[PLAYER2], username) == 0)
        {
            player = PLAYER2;
        }
        else
        {
            colorize("You are not a participant of this game.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Attempt to make the move
        int move_result = make_move(game, player, hole);
        if (move_result == -1)
        {
            colorize("Not your turn.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }
        else if (move_result == -2)
        {
            colorize("Not a hole you can select.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }
        else if (move_result == -3)
        {
            colorize("Selected hole is empty.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Prepare game state update message
        Message game_msg;
        game_msg.type = MSG_TYPE_TEXT;
        strcpy(game_msg.username, "Server");

        // Check if game is over
        if (move_result == 1)
        {
            snprintf(game_msg.data, BUFFER_SIZE, "Game %d over. Scores - %s: %d, %s: %d.",
                     game->game_id, game->player_usernames[PLAYER1], game->state.scores[PLAYER1],
                     game->player_usernames[PLAYER2], game->state.scores[PLAYER2]);
            // Notify both players
            send_to_user(game->player_usernames[PLAYER1], &game_msg);
            send_to_user(game->player_usernames[PLAYER2], &game_msg);

            // Remove the game from the list
            pthread_mutex_lock(&game_mutex);
            remove_game(&game_list, game->game_id);
            pthread_mutex_unlock(&game_mutex);
        }
        else
        {
            // Notify both players of the updated game state
            char *pos = game_msg.data;
            pos += sprintf(pos, " ===== Game %d =====\n", game->game_id);
            pos += sprintf(pos, "Move executed (%s played hole %d). It's %s's turn.\nNew board state:\n",
                           username, hole, game->player_usernames[game->state.turn]);
            pos += pretty_board_state(game, pos);
            // Todo: probs only need to send to the player whose turn it is
            pos += sprintf(pos, "%s, reply with /move %d <hole_number> to make your move.\n",
                           game->player_usernames[game->state.turn], game->game_id);
            send_to_user(game->player_usernames[PLAYER1], &game_msg);
            send_to_user(game->player_usernames[PLAYER2], &game_msg);
        }
    }
    else if (strcmp(command, "/listgames") == 0)
    {
        // List all active games the user is part of
        char list[BUFFER_SIZE] = "Active Games:\n";
        pthread_mutex_lock(&game_mutex);
        Game *current_game = game_list;
        while (current_game)
        {
            if (strcmp(current_game->player_usernames[PLAYER1], username) == 0 ||
                strcmp(current_game->player_usernames[PLAYER2], username) == 0)
            {
                char game_info[128];
                snprintf(game_info, sizeof(game_info), "Game ID: %d, Opponent: %s, Your Score: %d, Opponent Score: %d\n",
                         current_game->game_id,
                         strcmp(current_game->player_usernames[PLAYER1], username) == 0 ? current_game->player_usernames[PLAYER2] : current_game->player_usernames[PLAYER1],
                         current_game->state.scores[strcmp(current_game->player_usernames[PLAYER1], username) == 0 ? PLAYER1 : PLAYER2],
                         current_game->state.scores[strcmp(current_game->player_usernames[PLAYER1], username) == 0 ? PLAYER2 : PLAYER1]);
                strcat(list, game_info);
            }
            current_game = current_game->next;
        }
        pthread_mutex_unlock(&game_mutex);

        colorize(list, SERVER_GAME_STYLE, NULL, response.data);
        send_message(sockfd, &response);
    }
    else if (strncmp(command, "/gameinfo ", 10) == 0)
    {
        int game_id;
        sscanf(command + 10, "%d", &game_id);

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Check if user is part of the game
        if (strcmp(game->player_usernames[PLAYER1], username) != 0 &&
            strcmp(game->player_usernames[PLAYER2], username) != 0)
        {
            colorize("You are not a participant of this game.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Prepare game state information
        char info[BUFFER_SIZE];
        snprintf(info, sizeof(info),
                 "Game ID: %d\nPlayers: %s vs %s\nScores: %s: %d, %s: %d\nNext turn: %s\n",
                 game->game_id,
                 game->player_usernames[PLAYER1],
                 game->player_usernames[PLAYER2],
                 game->player_usernames[PLAYER1], game->state.scores[PLAYER1],
                 game->player_usernames[PLAYER2], game->state.scores[PLAYER2],
                 game->player_usernames[game->state.turn]);

        strcpy(response.data, info);
        send_message(sockfd, &response);
    }

    else
    {
        colorize("Unknown command.", SERVER_ERROR_STYLE, NULL, response.data);
        send_message(sockfd, &response);
    }
}

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
        colorize("Username already taken.", SERVER_ERROR_STYLE, NULL, response.data);
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
        colorize("Server full.", SERVER_ERROR_STYLE, NULL, response.data);
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }

    printf("%s has connected.\n", msg.username);
    Message welcome_msg;
    welcome_msg.type = MSG_TYPE_SERVER;
    colorize(SERVER_WELCOME_MESSAGE, SERVER_INFO_STYLE, NULL, welcome_msg.data);
    send_message(sockfd, &welcome_msg);
    // Also broadcast to other clients
    Message connected_msg;
    connected_msg.type = MSG_TYPE_SERVER;
    sprintf(connected_msg.data, "%s%s%s%s %shas connected.%s", SERVER_INFO_STYLE, STYLE_BOLD, msg.username, COLOR_RESET, SERVER_INFO_STYLE, COLOR_RESET);
    broadcast_message(&connected_msg, sockfd);

    // Main loop
    while (1)
    {
        res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            printf("%s has disconnected.\n", msg.username);
            break;
        }

        if (msg.data[0] == '/')
        {
            handle_command(sockfd, msg.data, msg.username);
        }
        else
        {
            // Broadcast message to other clients
            broadcast_message(&msg, sockfd);
        }
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
