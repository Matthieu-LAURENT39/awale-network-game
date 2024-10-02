#ifndef GAME_H
#define GAME_H

#define ROWS 2
#define COLUMNS 6

typedef struct
{
    int seeds[ROWS][COLUMNS];
    int score_player1;
    int score_player2;
} Board;

void initialize_board(Board *board);
void print_board(const Board *board);
int play_move(Board *board, int player, int hole);

#endif
