"""Game logic for the Awale game."""

from enum import Enum, auto


class Player(Enum):
    PLAYER1 = auto()
    PLAYER2 = auto()

    def __str__(self):
        return "Player 1" if self == Player.PLAYER1 else "Player 2"


class AwaleGame:
    def __init__(self):
        # Initialize the board with 4 seeds in each of the 12 pits
        self.board = [4] * 12
        self.scores: dict[Player, int] = {Player.PLAYER1: 0, Player.PLAYER2: 0}
        self.current_player: Player = Player.PLAYER1

    def is_valid_move(self, pit_index: int):
        # Check if the selected pit is valid for the current player
        if pit_index < 0 or pit_index > 11:
            return False
        if self.current_player == Player.PLAYER1 and pit_index > 5:
            return False
        if self.current_player == Player.PLAYER2 and pit_index < 6:
            return False
        if self.board[pit_index] == 0:
            return False
        return True

    def make_move(self, pit_index: int):
        # Perform the move and update the board and scores
        seeds = self.board[pit_index]
        self.board[pit_index] = 0
        index = pit_index

        while seeds > 0:
            index = (index + 1) % 12
            if index != pit_index:
                self.board[index] += 1
                seeds -= 1

        # Capture seeds
        captured = 0
        while self.board[index] in [2, 3] and (
            (self.current_player == Player.PLAYER1 and index > 5)
            or (self.current_player == Player.PLAYER2 and index < 6)
        ):
            captured += self.board[index]
            self.board[index] = 0
            index = (index - 1) % 12

        self.scores[self.current_player] += captured
        # Switch player
        self.current_player = (
            Player.PLAYER1 if self.current_player == Player.PLAYER2 else Player.PLAYER2
        )

    def get_board_state(self):
        return self.board.copy()

    def get_scores(self):
        return self.scores.copy()

    def is_game_over(self):
        # Check if the game is over by counting the number of empty pits
        return self.board.count(0) == 12

    def get_winner(self) -> Player | None:
        """Return the winner of the game, or None if it's a draw."""
        if self.scores[Player.PLAYER1] > self.scores[Player.PLAYER2]:
            return Player.PLAYER1
        if self.scores[Player.PLAYER1] < self.scores[Player.PLAYER2]:
            return Player.PLAYER2
        return None

    def pretty_board(self) -> str:
        """Return a pretty-printed version of the board."""
        board = self.get_board_state()
        return (
            "           | 11 | 10 | 9  | 8  | 7  | 6  |\n"
            f"Player 2: | {board[11]} | {board[10]} | {board[9]} | {board[8]} | {board[7]} | {board[6]} |\n"
            f"Player 1: | {board[0]} | {board[1]} | {board[2]} | {board[3]} | {board[4]} | {board[5]}\n"
            "           | 0  | 1  | 2  | 3  | 4  | 5  |"
        )
