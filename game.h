#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define NUM_HOLES 12             // Total number of holes on the board
#define INITIAL_SEEDS_PER_HOLE 4 // Initial seeds in each hole
#define MAX_WATCHERS 100         // Maximum number of watchers for a game

// Player identifiers
typedef enum
{
    PLAYER1 = 0,
    PLAYER2 = 1
} Player;

typedef enum
{
    ONGOING = 0,
    PLAYER1_WON = 1,
    PLAYER2_WON = 2,
    DRAW = 3
} GameStatus;

// Game state representation
typedef struct
{
    int board[NUM_HOLES]; // Seeds in each hole
    int scores[2];        // Scores for each player
    Player turn;          // Current player's turn (PLAYER1 or PLAYER2)
} GameState;

// Move history node
typedef struct MoveNode
{
    Player player;
    int hole;
    struct MoveNode *next;
} MoveNode;

// Game structure
typedef struct Game
{
    int game_id;
    // Player 1 is the player that goes first
    char player_usernames[2][USERNAME_MAX_LEN];
    GameState state;
    MoveNode *move_history;
    GameStatus status;
    int visibility; // 0 for private, 1 for public
    char watch_list[MAX_WATCHERS][USERNAME_MAX_LEN];
    struct Game *next; // For managing multiple games in a linked list
} Game;

// Function prototypes

// Game management
Game *create_game(int game_id, const char *player1_username, const char *player2_username);
void delete_game(Game *game);
void add_game(Game **game_list, Game *new_game);
Game *find_game_by_id(Game *game_list, int game_id);
void remove_game(Game **game_list, int game_id);

// Move management
int make_move(Game *game, int player, int hole);
int is_valid_move(Game *game, int player, int hole);

// Move history management
void add_move_to_history(Game *game, int player, int hole);
void free_move_history(MoveNode *history);

// Utility functions
int check_game_over(Game *game);
int pretty_board_state(Game *game, char *output);

// to string
char *game_to_string(Game *game);

// from string
Game *game_from_string(char *str);

#endif // GAME_H
