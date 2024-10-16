#include "common.h"
#include "game.h"
#include "color.h"
#include "server.h"
#include "user.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_CLIENTS 10

// Todo: factor out common logic

#define GAME_DIR "./games/"

// Forward definition
void save_game_state(Game *game);
void send_to_user(const char *username, Message *msg);

int next_game_id = 1;
// For matchmaking
char waiting_player[USERNAME_MAX_LEN] = "";

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

const char *SERVER_WELCOME_MESSAGE = "Welcome to Matt & Quent's Awale server!\nType /help for a list of available commands.";

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

void handle_matchmaking(int sockfd, const char *username)
{
    // MAKE SURE TO RELEASE THIS IN ALL CODE PATHS
    pthread_mutex_lock(&clients_mutex);
    if (strlen(waiting_player) == 0)
    {
        // No player is waiting, set current player as waiting
        strncpy(waiting_player, username, USERNAME_MAX_LEN - 1);
        waiting_player[USERNAME_MAX_LEN - 1] = '\0';
        Message msg;
        msg.type = MSG_TYPE_TEXT;
        strcpy(msg.username, "Server");
        strcpy(msg.data, "You are now in the matchmaking queue. Waiting for another player...");
        send_message(sockfd, &msg);
        pthread_mutex_unlock(&clients_mutex);
    }
    else
    {
        // Another player is waiting, start a game
        int game_id = next_game_id++;
        Game *new_game = create_game(game_id, waiting_player, username);
        if (new_game)
        {
            // Clear the waiting player
            char orig_waiting_player[USERNAME_MAX_LEN];
            strncpy(orig_waiting_player, waiting_player, USERNAME_MAX_LEN - 1);
            waiting_player[0] = '\0';

            add_game(&game_list, new_game);
            save_game_state(new_game);
            pthread_mutex_unlock(&clients_mutex);

            // Notify both players
            char game_start_msg[BUFFER_SIZE];
            sprintf(game_start_msg, "Match found! Game %d started between %s and %s.\n"
                                    "It's %s's turn.\n%s, reply with /move %d <hole_number> to make your move.",
                    game_id, new_game->player_usernames[PLAYER1], new_game->player_usernames[PLAYER2],
                    new_game->player_usernames[new_game->state.turn], new_game->player_usernames[new_game->state.turn], game_id);
            Message msg;
            msg.type = MSG_TYPE_TEXT;
            strcpy(msg.username, "Server");
            strcpy(msg.data, game_start_msg);
            send_message(sockfd, &msg);
            send_to_user(orig_waiting_player, &msg);

            // Send the initial board state
            msg.type = MSG_TYPE_INFO;
            strcpy(msg.data, game_to_string(new_game));
            send_message(sockfd, &msg);
            send_to_user(orig_waiting_player, &msg);
        }
        else
        {
            pthread_mutex_unlock(&clients_mutex);

            // Handle error in game creation
            Message msg;
            msg.type = MSG_TYPE_TEXT;
            strcpy(msg.username, "Server");
            strcpy(msg.data, "Failed to create game. Please try again later.");
            send_message(sockfd, &msg);
            send_to_user(waiting_player, &msg);
        }
    }
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

// ========== Filesystem logic ==========
void save_game_state(Game *game)
{
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/game_%d.dat", GAME_DIR, game->game_id);

    FILE *fp = fopen(filepath, "w");
    if (fp == NULL)
    {
        perror("Failed to open file for writing game state");
        return;
    }

    fprintf(fp, "%d|%s|%s|%d|%d|%d",
            game->game_id,
            game->player_usernames[PLAYER1],
            game->player_usernames[PLAYER2],
            game->state.scores[PLAYER1],
            game->state.scores[PLAYER2],
            game->state.turn);

    for (int i = 0; i < NUM_HOLES; i++)
    {
        fprintf(fp, "|%d", game->state.board[i]);
    }

    // Append the move history to the file
    MoveNode *node = game->move_history;
    while (node != NULL)
    {
        fprintf(fp, "|%d|%d", node->player, node->hole);
        node = node->next;
    }

    fclose(fp);
}

void load_all_games()
{
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(GAME_DIR)) == NULL)
    {
        perror("Failed to open directory");
        return;
    }

    int max_game_id = 0; // Local variable to find the maximum game ID

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        { // If the entry is a regular file
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s%s", GAME_DIR, entry->d_name);
            FILE *fp = fopen(filepath, "r");
            if (fp == NULL)
            {
                perror("Failed to open file for reading game state");
                continue;
            }

            Game *new_game = (Game *)malloc(sizeof(Game));
            if (new_game == NULL)
            {
                perror("Failed to allocate memory for game");
                fclose(fp);
                continue;
            }

            fscanf(fp, "%d|%[^|]|%[^|]|%d|%d|%d",
                   &new_game->game_id,
                   new_game->player_usernames[PLAYER1],
                   new_game->player_usernames[PLAYER2],
                   &new_game->state.scores[PLAYER1],
                   &new_game->state.scores[PLAYER2],
                   (int *)&new_game->state.turn);

            if (new_game->game_id > max_game_id)
            {
                max_game_id = new_game->game_id; // Update max_game_id if current game ID is higher
            }

            for (int i = 0; i < NUM_HOLES; i++)
            {
                fscanf(fp, "|%d", &new_game->state.board[i]);
            }

            // Load move history
            new_game->move_history = NULL;
            MoveNode **current_node = &new_game->move_history;
            while (fscanf(fp, "|%d|%d", (int *)&new_game->state.turn, &new_game->state.board[0]) == 2)
            {
                *current_node = (MoveNode *)malloc(sizeof(MoveNode));
                (*current_node)->player = new_game->state.turn;
                (*current_node)->hole = new_game->state.board[0];
                current_node = &(*current_node)->next;
            }
            *current_node = NULL;

            new_game->next = game_list;
            game_list = new_game;

            fclose(fp);

            printf("Loaded game %d\n", new_game->game_id);
        }
    }

    closedir(dir);

    next_game_id = max_game_id + 1; // Set next_game_id to one more than the highest found
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
    else if (strncmp(command, "/forfeit", 8) == 0)
    {
        int game_id;
        sscanf(command + 8, "%d", &game_id);
        pthread_mutex_lock(&game_mutex);
        Game *game_to_forfeit = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game_to_forfeit)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
        else
        {
            // Store the game state before closing it
            pthread_mutex_lock(&game_mutex);
            game_to_forfeit->status = (strcmp(username, game_to_forfeit->player_usernames[PLAYER1]) == 0) ? PLAYER2_WON : PLAYER1_WON;
            save_game_state(game_to_forfeit);
            pthread_mutex_unlock(&game_mutex);

            // send message to both players
            Message forfeit_msg;
            forfeit_msg.type = MSG_TYPE_SERVER;
            strcpy(forfeit_msg.username, "Server");
            sprintf(forfeit_msg.data, "Game %d has been forfeited by %s.", game_id, username);
            send_to_user(game_to_forfeit->player_usernames[PLAYER1], &forfeit_msg);
            send_to_user(game_to_forfeit->player_usernames[PLAYER2], &forfeit_msg);
        }
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
                               "/history - Show the move history of the current game\n"
                               "/gameinfo <game_id> - Get detailed information about a specific game\n"
                               "/forfeit <game_id> - Forfeit a game\n"
                               "/exit - Disconnect from the server\n"
                               "/watch <game_id> - Watch a game\n"
                               "/unwatch <game_id> - Stop watching a game\n"
                               "/info <username> - Get information about a user (his name and biography)\n"
                               "/match - Join the matchmaking queue\n"
                               "/visibility <game_id> <visibility> - Set the visibility of a game (0 for private, 1 for public)\n"
                               "/addfriend <username> - Add a user to your friends list\n"
                               "/removefriend <username> - Remove a user from your friends list\n"
                               "/getfriends - List your friends\n"
                               "/bio <biography> - Set your biography\n%s",
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
        // Save the game state to a file
        save_game_state(new_game);
        pthread_mutex_unlock(&game_mutex);

        // Notify both players
        Message game_start_msg;
        game_start_msg.type = MSG_TYPE_TEXT;
        strcpy(game_start_msg.username, "Server");
        char *pos = game_start_msg.data;

        // make a random first player
        new_game->state.turn = rand() % 2;

        pos += sprintf(pos, "Game %d started between %s%s%s and %s%s%s. It's %s's turn.\n",
                       game_id, STYLE_BOLD, new_game->player_usernames[PLAYER1], COLOR_RESET, STYLE_BOLD,
                       new_game->player_usernames[PLAYER2], COLOR_RESET, new_game->player_usernames[new_game->state.turn]);
        // Todo: probs only need to send to the player whose turn it is
        pos += sprintf(pos, "%s, reply with /move %d <hole_number> to make your move.\n",
                       new_game->player_usernames[new_game->state.turn], game_id);
        send_to_user(new_game->player_usernames[PLAYER1], &game_start_msg);
        send_to_user(new_game->player_usernames[PLAYER2], &game_start_msg);
        free(challenge);

        // pretty_board_state(new_game, response.data);
        // send_to_user(new_game->player_usernames[PLAYER1], &response);
        // send_to_user(new_game->player_usernames[PLAYER2], &response);

        // Print the initial board state
        game_start_msg.type = MSG_TYPE_INFO;
        strcpy(game_start_msg.username, "Server");
        strcpy(game_start_msg.data, game_to_string(new_game));
        send_to_user(new_game->player_usernames[PLAYER1], &game_start_msg);
        send_to_user(new_game->player_usernames[PLAYER2], &game_start_msg);
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

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            pthread_mutex_unlock(&game_mutex);
            return;
        }

        // Check the game isn't over
        if (game->status != ONGOING)
        {
            colorize("Game is already over.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            pthread_mutex_unlock(&game_mutex);
            return;
        }
        pthread_mutex_unlock(&game_mutex);

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
            // pos += pretty_board_state(game, pos);
            // Todo: probs only need to send to the player whose turn it is
            pos += sprintf(pos, "%s, reply with /move %d <hole_number> to make your move.\n",
                           game->player_usernames[game->state.turn], game->game_id);
            send_to_user(game->player_usernames[PLAYER1], &game_msg);
            send_to_user(game->player_usernames[PLAYER2], &game_msg);

            game_msg.type = MSG_TYPE_INFO;
            strcpy(game_msg.username, "Server");
            strcpy(game_msg.data, game_to_string(game));
            send_to_user(game->player_usernames[PLAYER1], &game_msg);
            send_to_user(game->player_usernames[PLAYER2], &game_msg);

            for (int i = 0; i < MAX_WATCHERS; i++)
            {
                if (game->watch_list[i][0] != '\0')
                {
                    send_to_user(game->watch_list[i], &game_msg);
                }
            }
        }

        // Save the game state to a file
        pthread_mutex_lock(&game_mutex);
        save_game_state(game);
        pthread_mutex_unlock(&game_mutex);
    }
    else if (strcmp(command, "/listgames") == 0)
    {
        // List all active games, with a special message if the user is a participant
        char list[BUFFER_SIZE] = "Active Games:\n";
        pthread_mutex_lock(&game_mutex);
        Game *current = game_list;
        while (current)
        {
            char *pos = list + strlen(list);
            if (strcmp(current->player_usernames[PLAYER1], username) == 0 ||
                strcmp(current->player_usernames[PLAYER2], username) == 0)
            {
                pos += sprintf(pos, "[YOU] ");
            }
            pos += sprintf(pos, "Game %d: %s vs %s (", current->game_id,
                           current->player_usernames[PLAYER1], current->player_usernames[PLAYER2]);
            if (current->status == ONGOING)
            {
                pos += sprintf(pos, "ongoing");
            }
            else if (current->status == PLAYER1_WON)
            {
                pos += sprintf(pos, "%s won", current->player_usernames[PLAYER1]);
            }
            else if (current->status == PLAYER2_WON)
            {
                pos += sprintf(pos, "%s won", current->player_usernames[PLAYER2]);
            }
            else if (current->status == DRAW)
            {
                pos += sprintf(pos, "draw");
            }
            pos += sprintf(pos, ")\n");
            current = current->next;
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

        // Prepare game state information
        Message game_msg;
        game_msg.type = MSG_TYPE_INFO;
        strcpy(game_msg.username, "Server");
        strcpy(game_msg.data, game_to_string(game));
        send_message(sockfd, &game_msg);
    }
    else if (strncmp(command, "/visibility", 11) == 0)
    {
        int game_id;
        int visibility;
        sscanf(command + 11, "%d %d", &game_id, &visibility);

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        if (strcmp(username, game->player_usernames[PLAYER1]) != 0)
        {
            colorize("You are not the host of this game.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // check if visibility is equal to 0 or 1, other is incorrect
        if (visibility != 0 && visibility != 1)
        {
            colorize("Visibility must be 0 or 1.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        game->visibility = visibility;
        colorize("Visibility updated.", SERVER_SUCCESS_STYLE, NULL, response.data);
        send_message(sockfd, &response);
    }
    // Get the history of moves in a game
    else if (strncmp(command, "/history ", 9) == 0)
    {
        int game_id;
        sscanf(command + 9, "%d", &game_id);

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Prepare move history information
        Message history_msg;
        history_msg.type = MSG_TYPE_TEXT;
        strcpy(history_msg.username, "Server");
        MoveNode *current = game->move_history;
        char *pos = history_msg.data;
        pos += sprintf(pos, "Move history for game %d:\n", game_id);
        while (current)
        {
            pos += sprintf(pos, "%s played hole %d\n", game->player_usernames[current->player], current->hole + 1);
            current = current->next;
        }
        send_message(sockfd, &history_msg);
    }
    // add friend command
    else if (strncmp(command, "/addfriend", 10) == 0)
    {
        char friend_username[USERNAME_MAX_LEN];
        sscanf(command + 11, "%s", friend_username);

        if (strcmp(friend_username, username) == 0)
        {
            colorize("You cannot add yourself as a friend.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Check if friend user exists
        pthread_mutex_lock(&clients_mutex);
        int user_found = user_exists(friend_username);
        pthread_mutex_unlock(&clients_mutex);

        if (!user_found)
        {
            colorize("User not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Load the user
        User user;
        if (load_user(username, &user) == 1)
        {
            // Add the friend
            int added = 0;
            for (int i = 0; i < MAX_FRIENDS; i++)
            {
                if (user.friends[i][0] == '\0')
                {
                    strncpy(user.friends[i], friend_username, USERNAME_MAX_LEN - 1);
                    user.friends[i][USERNAME_MAX_LEN - 1] = '\0';
                    added = 1;
                    break;
                }
            }

            if (added)
            {
                if (save_user(&user) == 1)
                {
                    colorize("Friend added successfully.", SERVER_SUCCESS_STYLE, NULL, response.data);
                    send_message(sockfd, &response);
                }
                else
                {
                    colorize("Failed to save user.", SERVER_ERROR_STYLE, NULL, response.data);
                    send_message(sockfd, &response);
                }
            }
            else
            {
                colorize("Friend list is full.", SERVER_ERROR_STYLE, NULL, response.data);
                send_message(sockfd, &response);
            }
        }
        else
        {
            colorize("Failed to load user.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    else if (strncmp(command, "/removefriend", 12) == 0)
    {
        char friend_username[USERNAME_MAX_LEN];
        sscanf(command + 13, "%s", friend_username);

        if (strcmp(friend_username, username) == 0)
        {
            colorize("You cannot remove yourself as a friend.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Check if friend user exists
        pthread_mutex_lock(&clients_mutex);
        int user_found = user_exists(friend_username);
        pthread_mutex_unlock(&clients_mutex);

        if (!user_found)
        {
            colorize("User not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Load the user
        User user;
        if (load_user(username, &user) == 1)
        {
            // Add the friend
            int removed = remove_friend(username, friend_username);

            if (removed)
            {
                colorize("Friend removed successfully.", SERVER_SUCCESS_STYLE, NULL, response.data);
                send_message(sockfd, &response);
            }
            else
            {
                colorize("Friend not found in your list.", SERVER_ERROR_STYLE, NULL, response.data);
                send_message(sockfd, &response);
            }
        }
        else
        {
            colorize("Failed to load user.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    else if (strcmp(command, "/getfriends") == 0)
    {
        User user;
        if (load_user(username, &user) == 1)
        {
            char friends_list[BUFFER_SIZE] = "Friends:\n";
            for (int i = 0; i < MAX_FRIENDS; i++)
            {
                if (user.friends[i][0] != '\0')
                {
                    strcat(friends_list, user.friends[i]);
                    strcat(friends_list, "\n");
                }
            }
            colorize(friends_list, SERVER_SUCCESS_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
        else
        {
            colorize("Failed to load user.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    // watch a specific game
    else if (strncmp(command, "/watch ", 7) == 0)
    {
        int game_id;
        sscanf(command + 7, "%d", &game_id);

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Add the user to the watch list
        pthread_mutex_lock(&game_mutex);
        // if the game is private, the user can't watch it if they are not a friend of the players
        // We check both players' friends list, since friendship is unilateral, use function is_friend to check if the user is a friend of the player
        if (game->visibility == 0)
        {
            if (!is_friend(game->player_usernames[PLAYER1], username) && !is_friend(game->player_usernames[PLAYER2], username))
            {
                colorize("You can't watch this game because it's private and you are not a friend of the players.", SERVER_ERROR_STYLE, NULL, response.data);
                send_message(sockfd, &response);
                pthread_mutex_unlock(&game_mutex);
                return;
            }
        }
        // check if the user is already watching the game
        int already_watching = 0;
        for (int i = 0; i < 100; i++)
        {
            if (strcmp(game->watch_list[i], username) == 0)
            {
                colorize("You are already watching this game.", SERVER_ERROR_STYLE, NULL, response.data);
                already_watching = 1;
                send_message(sockfd, &response);
                break;
            }
        }

        // check if user doesn't watch his own game
        int own_game = 0;
        if (strcmp(game->player_usernames[PLAYER1], username) == 0 || strcmp(game->player_usernames[PLAYER2], username) == 0)
        {
            colorize("You can't watch your own game.", SERVER_ERROR_STYLE, NULL, response.data);
            own_game = 1;
            send_message(sockfd, &response);
        }
        // traverse the watch list to see an available slot
        if (already_watching == 0 && own_game == 0)
        {
            for (int i = 0; i < 100; i++)
            {
                if (game->watch_list[i][0] == '\0')
                {
                    strncpy(game->watch_list[i], username, USERNAME_MAX_LEN - 1);
                    colorize("You are now watching the game.", SERVER_SUCCESS_STYLE, NULL, response.data);
                    send_message(sockfd, &response);
                    break;
                }
            }
        }
        pthread_mutex_unlock(&game_mutex);
    }
    else if (strncmp(command, "/unwatch ", 7) == 0)
    {
        int game_id;
        sscanf(command + 9, "%d", &game_id);

        pthread_mutex_lock(&game_mutex);
        Game *game = find_game_by_id(game_list, game_id);
        pthread_mutex_unlock(&game_mutex);

        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        // Remove the user from the watch list
        pthread_mutex_lock(&game_mutex);
        int watching = 0;
        for (int i = 0; i < 100; i++)
        {
            if (strcmp(game->watch_list[i], username) == 0)
            {
                game->watch_list[i][0] = '\0';
                watching = 1;
                colorize("You are no longer watching the game.", SERVER_SUCCESS_STYLE, NULL, response.data);
                break;
            }
        }

        if (!watching)
        {
            colorize("You are not watching this game.", SERVER_ERROR_STYLE, NULL, response.data);
        }

        pthread_mutex_unlock(&game_mutex);
        send_message(sockfd, &response);
    }
    // chat to a party with /chat <number_of_party> <message>
    else if (strncmp(command, "/chat ", 6) == 0)
    {
        int party;
        char message[BUFFER_SIZE];
        sscanf(command + 6, "%d %[^\n]", &party, message);
        Game *game = find_game_by_id(game_list, party);
        if (!game)
        {
            colorize("Game not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        if (strcmp(username, game->player_usernames[PLAYER1]) == 0 || strcmp(username, game->player_usernames[PLAYER2]) == 0)
        {
            Message chat_msg;
            chat_msg.type = MSG_TYPE_GAME;
            strcpy(chat_msg.username, username);
            strcpy(chat_msg.data, message);

            send_to_user(game->player_usernames[PLAYER1], &chat_msg);
            send_to_user(game->player_usernames[PLAYER2], &chat_msg);
        }
        else
        {
            colorize("You are not a participant of this game.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    // Allow user to set a biography
    else if (strncmp(command, "/bio ", 5) == 0)
    {
        const char *new_bio = command + 5;

        // Load the user
        User user;
        if (load_user(username, &user) == 1)
        {
            // Update the biography
            strncpy(user.biography, new_bio, sizeof(user.biography) - 1);
            user.biography[sizeof(user.biography) - 1] = '\0';

            if (save_user(&user) == 1)
            {
                colorize("Biography updated successfully.", SERVER_SUCCESS_STYLE, NULL, response.data);
                send_message(sockfd, &response);
            }
            else
            {
                colorize("Failed to save biography.", SERVER_ERROR_STYLE, NULL, response.data);
                send_message(sockfd, &response);
            }
        }
        else
        {
            colorize("Failed to load user data.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    // get info of a specific user, get the informations from the file of the user, don't show the password
    else if (strncmp(command, "/info ", 6) == 0)
    {
        char target_username[USERNAME_MAX_LEN];
        sscanf(command + 6, "%s", target_username);

        User target_user;
        if (load_user(target_username, &target_user) == 1)
        {
            // Prepare response message
            snprintf(response.data, BUFFER_SIZE, "%sUsername: %s%s\n%sBiography: %s%s",
                     SERVER_INFO_STYLE, target_user.username, COLOR_RESET,
                     SERVER_INFO_STYLE, target_user.biography, COLOR_RESET);
            send_message(sockfd, &response);
        }
        else
        {
            colorize("User not found.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
        }
    }
    // private message with /mp <name_to_receiver> <message>
    else if (strncmp(command, "/mp ", 4) == 0)
    {
        char receiver[USERNAME_MAX_LEN];
        char message[BUFFER_SIZE];
        sscanf(command + 4, "%s %[^\n]", receiver, message);

        if (strcmp(username, receiver) == 0)
        {
            colorize("You can't send a message to yourself.", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            return;
        }

        Message private_msg;
        private_msg.type = MSG_TYPE_MP;
        strcpy(private_msg.username, username);
        strcpy(private_msg.data, message);

        send_to_user(receiver, &private_msg);
    }
    else if (strcmp(command, "/match") == 0)
    {
        handle_matchmaking(sockfd, username);
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

    if (res == -1 || msg.type == MSG_TYPE_EXIT)
    {
        close(sockfd);
        pthread_exit(NULL);
    }
    if (is_username_taken(msg.username))
    {
        // Username is taken
        Message response;
        response.type = MSG_TYPE_EXIT;
        sprintf(response.data, "Username %s is already taken.", msg.username);
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }
    // Validate username length
    if (strlen(msg.username) == 0 || strlen(msg.username) >= USERNAME_MAX_LEN)
    {
        // Invalid username
        Message response;
        response.type = MSG_TYPE_EXIT;
        sprintf(response.data, "Invalid username. Must be between 1 and %d characters.", USERNAME_MAX_LEN - 1);
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }
    // Only allow alphanumeric usernames
    for (int i = 0; i < strlen(msg.username); i++)
    {
        if (!isalnum(msg.username[i]))
        {
            // Invalid username
            Message response;
            response.type = MSG_TYPE_EXIT;
            sprintf(response.data, "Invalid username. Must be alphanumeric.");
            send_message(sockfd, &response);
            close(sockfd);
            pthread_exit(NULL);
        }
    }

    User user;
    int user_exists = load_user(msg.username, &user);

    if (user_exists == 1)
    {
        // User exists, check password
        Message response;
        response.type = MSG_TYPE_SERVER;
        colorize("Password: ", SERVER_INFO_STYLE, NULL, response.data);
        send_message(sockfd, &response);
        res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            close(sockfd);
            pthread_exit(NULL);
        }

        // if user enters wrong password repeat until correct
        while (strcmp(msg.data, user.password) != 0)
        {
            Message response;
            response.type = MSG_TYPE_SERVER;
            colorize("Incorrect password. Try again: ", SERVER_ERROR_STYLE, NULL, response.data);
            send_message(sockfd, &response);
            res = receive_message(sockfd, &msg);
            if (res == -1 || msg.type == MSG_TYPE_EXIT)
            {
                close(sockfd);
                pthread_exit(NULL);
            }
        }
    }
    else if (user_exists == 0)
    {
        // User does not exist, create new user
        Message response;
        response.type = MSG_TYPE_SERVER;
        colorize("Create Password: ", SERVER_INFO_STYLE, NULL, response.data);
        send_message(sockfd, &response);
        res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            close(sockfd);
            pthread_exit(NULL);
        }

        // Copy the password to user struct
        strcpy(user.username, msg.username);
        strcpy(user.password, msg.data);

        // ask for a biography to the user
        response.type = MSG_TYPE_SERVER;
        colorize("Biography: ", SERVER_INFO_STYLE, NULL, response.data);
        send_message(sockfd, &response);
        res = receive_message(sockfd, &msg);
        if (res == -1 || msg.type == MSG_TYPE_EXIT)
        {
            close(sockfd);
            pthread_exit(NULL);
        }
        strcpy(user.biography, msg.data);

        // Save the new user
        save_user(&user);
    }
    else
    {
        // Error loading user
        Message response;
        response.type = MSG_TYPE_EXIT;
        sprintf(response.data, "Error loading user data.");
        send_message(sockfd, &response);
        close(sockfd);
        pthread_exit(NULL);
    }

    // Send welcome message
    Message welcome_msg;
    welcome_msg.type = MSG_TYPE_SERVER;
    colorize("Connection successful", SERVER_SUCCESS_STYLE, STYLE_BOLD, welcome_msg.data);
    send_message(sockfd, &welcome_msg);

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
    // Clear the waiting player if they disconnect
    if (strcmp(clients[i].username, waiting_player) == 0)
    {
        waiting_player[0] = '\0';
    }
    clients[i].sockfd = 0;
    clients[i].username[0] = '\0';
    pthread_mutex_unlock(&clients_mutex);

    close(sockfd);
    pthread_exit(NULL);
}

int main()
{
    // Create games directory if it doesn't exist
    mkdir(GAME_DIR, 0755);

    // Load all games from the filesystem
    load_all_games();

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
