#include "game.h"
#include <stdio.h>

void initialize_board(Board *board)
{
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLUMNS; j++)
        {
            board->seeds[i][j] = 4;
        }
    }
    board->score_player1 = 0;
    board->score_player2 = 0;
}

void print_board(const Board *board)
{
    printf("Player 2: \n");
    for (int j = COLUMNS - 1; j >= 0; j--)
    {
        printf("%2d ", board->seeds[1][j]);
    }
    printf("\nPlayer 1: \n");
    for (int j = 0; j < COLUMNS; j++)
    {
        printf("%2d ", board->seeds[0][j]);
    }
    printf("\n");
}

int play_move(Board *board, int player, int hole)
{
    // Ajoutez votre logique de coup ici
    return 0; // Modifier avec une vérification correcte
}
