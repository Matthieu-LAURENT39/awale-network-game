// Note that all holes are 0-indexed, except when displaying the board to the user.

#include "game.h"
#include "common.h"

// Create a new game instance
Game *create_game(int game_id, const char *player1_username, const char *player2_username)
{
    Game *game = (Game *)malloc(sizeof(Game));
    if (!game)
    {
        perror("Failed to allocate memory for new game");
        return NULL;
    }

    game->game_id = game_id;
    strncpy(game->player_usernames[PLAYER1], player1_username, USERNAME_MAX_LEN - 1);
    game->player_usernames[PLAYER1][USERNAME_MAX_LEN - 1] = '\0';
    strncpy(game->player_usernames[PLAYER2], player2_username, USERNAME_MAX_LEN - 1);
    game->player_usernames[PLAYER2][USERNAME_MAX_LEN - 1] = '\0';

    // Initialize the board with initial seeds
    for (int i = 0; i < NUM_HOLES; i++)
    {
        game->state.board[i] = INITIAL_SEEDS_PER_HOLE;
    }

    game->state.scores[PLAYER1] = 0;
    game->state.scores[PLAYER2] = 0;
    game->state.turn = PLAYER1; // Player 1 starts the game
    game->move_history = NULL;
    game->next = NULL;

    return game;
}

// Delete a game instance and free resources
void delete_game(Game *game)
{
    if (game)
    {
        free_move_history(game->move_history);
        free(game);
    }
}

// Add a game to the linked list of games
void add_game(Game **game_list, Game *new_game)
{
    new_game->next = *game_list;
    *game_list = new_game;
}

// Find a game by its unique game ID
Game *find_game_by_id(Game *game_list, int game_id)
{
    Game *current = game_list;
    while (current)
    {
        if (current->game_id == game_id)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Remove a game from the linked list and delete it
void remove_game(Game **game_list, int game_id)
{
    Game *current = *game_list;
    Game *prev = NULL;

    while (current)
    {
        if (current->game_id == game_id)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                *game_list = current->next;
            }
            delete_game(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Add a move to the game's move history
void add_move_to_history(Game *game, int player, int hole)
{
    MoveNode *move = (MoveNode *)malloc(sizeof(MoveNode));
    if (!move)
    {
        perror("Failed to allocate memory for move history");
        return;
    }
    move->player = player;
    move->hole = hole;
    move->next = game->move_history;
    game->move_history = move;
}

// Free the move history linked list
void free_move_history(MoveNode *history)
{
    while (history)
    {
        MoveNode *temp = history;
        history = history->next;
        free(temp);
    }
}

// Check if a move is valid
// Returns:
//  0 - Valid move
//  1 - Not this player's turn
//  2 - Invalid hole for the player
//  3 - Selected hole is empty
int is_valid_move(Game *game, int player, int hole)
{
    if (game->state.turn != player)
    {
        // Not this player's turn
        return 1;
    }

    if (player == PLAYER1 && (hole < 0 || hole >= NUM_HOLES / 2))
    {
        // Invalid hole for Player 1
        return 2;
    }

    if (player == PLAYER2 && (hole < NUM_HOLES / 2 || hole >= NUM_HOLES))
    {
        // Invalid hole for Player 2
        return 2;
    }

    if (game->state.board[hole] == 0)
    {
        // Cannot select an empty hole
        return 3;
    }

    return 0;
}

// Execute a move in the game
// Returns:
//  0 - Move executed successfully
//  1 - Game over
// Invalid moves: (Negative values, see is_valid_move)
// -1 - Not this player's turn
// -2 - Invalid hole for the player
// -3 - Selected hole is empty
int make_move(Game *game, int player, int hole)
{
    int valid = is_valid_move(game, player, hole);
    if (valid != 0)
    {
        return -valid;
    }

    int seeds = game->state.board[hole];
    game->state.board[hole] = 0;
    int position = hole;

    // Sow seeds in subsequent holes
    while (seeds > 0)
    {
        position = (position + 1) % NUM_HOLES;
        // Skip opponent's scoring hole if implementing scoring pits
        game->state.board[position]++;
        seeds--;
    }

    // Capture logic
    int captured = 0;
    while (1)
    {
        if ((player == PLAYER1 && position >= NUM_HOLES / 2) ||
            (player == PLAYER2 && position < NUM_HOLES / 2))
        {
            // Check opponent's side for capture
            if (game->state.board[position] == 2 || game->state.board[position] == 3)
            {
                captured += game->state.board[position];
                game->state.board[position] = 0;
                position = (position - 1 + NUM_HOLES) % NUM_HOLES;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    game->state.scores[player] += captured;

    // Add move to history
    add_move_to_history(game, player, hole);

    // Check for game over
    if (check_game_over(game))
    {
        // Game over logic
        // Distribute remaining seeds to the opponent
        for (int i = 0; i < NUM_HOLES; i++)
        {
            int owner = (i < NUM_HOLES / 2) ? PLAYER1 : PLAYER2;
            game->state.scores[owner] += game->state.board[i];
            game->state.board[i] = 0;
        }
        // Indicate game over
        return 1;
    }

    // Change turn to the other player
    game->state.turn = 1 - game->state.turn;

    return 0; // Move executed successfully
}

// Print the current game board
// Returns the number of characters written to the output buffer
int pretty_board_state(Game *game, char *output)
{
    int *board = game->state.board;
    int *scores = game->state.scores;
    char *pos = output; // Pointer to track position in the output buffer

    pos += sprintf(pos, "\n  Player 2 (%s) - Score: %d\n", game->player_usernames[PLAYER2], scores[PLAYER2]);
    pos += sprintf(pos, "  +----+----+----+----+----+----+\n");
    pos += sprintf(pos, "  | 12 | 11 | 10 | 9  | 8  | 7  |\n");
    pos += sprintf(pos, "  | %2d | %2d | %2d | %2d | %2d | %2d |\n", board[11], board[10], board[9], board[8], board[7], board[6]);
    pos += sprintf(pos, "  +----+----+----+----+----+----+\n");

    pos += sprintf(pos, "  +----+----+----+----+----+----+\n");
    pos += sprintf(pos, "  | 1  | 2  | 3  | 4  | 5  | 6  |\n");
    pos += sprintf(pos, "  | %2d | %2d | %2d | %2d | %2d | %2d |\n", board[0], board[1], board[2], board[3], board[4], board[5]);
    pos += sprintf(pos, "  +----+----+----+----+----+----+\n");
    pos += sprintf(pos, "  Player 1 (%s) - Score: %d\n\n", game->player_usernames[PLAYER1], scores[PLAYER1]);
    return pos - output;
}

// Check if the game is over
int check_game_over(Game *game)
{
    int side1_empty = 1;
    int side2_empty = 1;

    // Check if Player 1's side is empty
    for (int i = 0; i < NUM_HOLES / 2; i++)
    {
        if (game->state.board[i] > 0)
        {
            side1_empty = 0;
            break;
        }
    }

    // Check if Player 2's side is empty
    for (int i = NUM_HOLES / 2; i < NUM_HOLES; i++)
    {
        if (game->state.board[i] > 0)
        {
            side2_empty = 0;
            break;
        }
    }

    // Game is over if one side is empty
    return side1_empty || side2_empty;
}
