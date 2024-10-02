import socket
import threading
from dataclasses import dataclass
from io import StringIO

from common import PORT, heartbeat, receive, send, validate_pseudo
from game import AwaleGame, Player
from network_data import (
    ClientMessageData,
    ConnectionData,
    ConnectionResponse,
    MessageData,
)

HOST: str = "localhost"

MOTD: str = "Welcome to the Awale server!\n"
"""Display message for clients when they connect."""


@dataclass
class Client:
    conn: socket.socket

    pseudo: str
    platform: str

    current_game: AwaleGame | None = None


class GameController:
    def __init__(self):
        self.connections: set[socket.socket] = set()
        self.clients: dict[str, socket.socket] = {}
        self.games: dict[str, dict] = {}
        self.lock: threading.RLock = threading.RLock()

    def accept_connection(self, conn: socket.socket) -> None:
        with self.lock:
            self.connections.add(conn)

    def register_client(self, conn: socket.socket, conn_data: ConnectionData) -> bool:
        with self.lock:
            if not validate_pseudo(conn_data.pseudo):
                send(
                    conn,
                    ConnectionResponse(
                        success=False,
                        message="Pseudo must be alphanumeric and at most 10 characters.\nPlease choose another pseudo.",
                    ),
                )
                return False

            if conn_data.pseudo in self.clients:
                send(
                    conn,
                    ConnectionResponse(
                        success=False,
                        message="Pseudo already taken. Please choose another pseudo.",
                    ),
                )
                return False

            self.clients[conn_data.pseudo] = conn
            send(conn, ConnectionResponse(success=True, message=None))
            return True

    def list_available_players(self, pseudo: str) -> str:
        message = StringIO()
        message.write(" Available players ".center(40, "=") + "\n")
        with self.lock:
            for player in self.clients:
                message.write(f"{player}")
                if player == pseudo:
                    message.write(" (you)")
                if player in self.games:
                    message.write(" (in game)")
                message.write("\n")
        return message.getvalue()

    def broadcast_message(
        self,
        message: str,
        sender_pseudo: str | None = None,
        is_server_message: bool = False,
    ) -> None:
        """Broadcast a message to all clients except the sender."""
        print(message)
        with self.lock:
            for pseudo, conn in self.clients.items():
                if pseudo != sender_pseudo:
                    send(
                        conn,
                        MessageData(
                            username=sender_pseudo,
                            message=message,
                            is_server_message=is_server_message,
                        ),
                    )

    def send_message(
        self,
        message: str,
        from_pseudo: str,
        to_pseudo: str,
        is_server_message: bool = False,
    ) -> None:
        """Send a message to a specific client."""
        with self.lock:
            if to_pseudo in self.clients:
                send(
                    self.clients[to_pseudo],
                    MessageData(
                        username=from_pseudo,
                        message=message,
                        is_server_message=is_server_message,
                    ),
                )
            else:
                print(
                    f"[WARNING] Tried sending message to nonexistend pseudo {to_pseudo}."
                )

    def start_game(self, pseudo1: str, pseudo2: str) -> None:
        with self.lock:
            game_id: str = f"{pseudo1}_vs_{pseudo2}"
            game = AwaleGame()
            self.games[game_id] = {
                "players": {Player.PLAYER1: pseudo1, Player.PLAYER2: pseudo2},
                "game": game,
            }
        first_player: str = pseudo1 if pseudo1 < pseudo2 else pseudo2
        second_player: str = pseudo2 if first_player == pseudo1 else pseudo1
        with self.lock:
            self.send_message(
                f"Game started with {pseudo2}. You are {'first' if first_player == pseudo1 else 'second'}.\n",
                "GAME",
                pseudo1,
                is_server_message=True,
            )
            self.send_message(
                f"Game started with {pseudo1}. You are {'first' if first_player == pseudo2 else 'second'}.\n",
                "GAME",
                pseudo2,
                is_server_message=True,
            )
        self.send_board_state(game_id)

    def send_board_state(self, game_id: str) -> None:
        with self.lock:
            game_info = self.games[game_id]
            board = game_info["game"].pretty_board()
            scores = game_info["game"].get_scores()
            for player in game_info["players"].values():
                self.send_message(
                    f"Board:\n{board}\n\nScores: {scores}\n",
                    "GAME",
                    player,
                    is_server_message=True,
                )

    def handle_game_action(self, pseudo: str, pit_index: int) -> None:
        """Handle a game action from a player."""

        with self.lock:
            for game_id, game_info in self.games.items():
                if pseudo in game_info["players"].values():
                    game = game_info["game"]
                    if game_info["players"][game.current_player] == pseudo:
                        try:
                            if game.is_valid_move(pit_index):
                                game.make_move(pit_index)
                                self.send_board_state(game_id)
                                if game.is_game_over():
                                    winner = game.get_winner()
                                    message = (
                                        f"{game_info['players'][winner]} wins the game!\n"
                                        if winner != -1
                                        else "The game is a draw.\n"
                                    )
                                    for player in game_info["players"].values():
                                        self.send_message(
                                            message,
                                            "GAME",
                                            player,
                                            is_server_message=True,
                                        )
                                    del self.games[game_id]
                                break
                            else:
                                self.send_message(
                                    "Invalid move. Try again.\n",
                                    "GAME",
                                    pseudo,
                                    is_server_message=True,
                                )
                        except ValueError:
                            self.send_message(
                                "Please enter a valid pit number.\n",
                                "GAME",
                                pseudo,
                                is_server_message=True,
                            )
                    else:
                        self.send_message(
                            "It's not your turn.\n",
                            "GAME",
                            pseudo,
                            is_server_message=True,
                        )
                    break


def handle_client(conn: socket.socket, controller: GameController) -> None:
    """Per-client loop to handle client messages."""

    # Register the client
    # We loop until the client sends valid data
    conn_data = ConnectionData.deserialize(receive(conn))
    while not controller.register_client(conn, conn_data):
        conn_data = ConnectionData.deserialize(receive(conn))

    pseudo = conn_data.pseudo

    controller.send_message(MOTD, "MOTD", pseudo, is_server_message=True)
    controller.send_message(
        f"Hello {pseudo}! You can type /list to see available players.\n",
        "SERVER",
        pseudo,
        is_server_message=True,
    )
    controller.broadcast_message(f"{pseudo} has joined the server!", "SERVER", True)

    while True:
        raw_data = receive(conn)
        data = ClientMessageData.deserialize(raw_data)
        msg = data.message

        # Handle server commands
        if msg.startswith("/"):
            cmd, *args = msg.removeprefix("/").split()
            # Ensure case-insensitive commands
            cmd = cmd.casefold()
            # Remove empty strings from args
            args = [arg for arg in args if arg]

            if cmd == "quit":
                break

            elif cmd == "help":
                controller.send_message(
                    "Available commands:\n/list - List available players\n/challenge <player> - Start a game with a player\n",
                    "SERVER",
                    pseudo,
                    is_server_message=True,
                )

            if cmd == "list":
                controller.send_message(
                    controller.list_available_players(pseudo),
                    "SERVER",
                    pseudo,
                    is_server_message=True,
                )

            elif cmd == "challenge":
                if len(args) != 1:
                    controller.send_message(
                        "Usage: /challenge <player>\n",
                        "SERVER",
                        pseudo,
                        is_server_message=True,
                    )
                    continue

                target_pseudo = msg.split(" ")[1]
                if target_pseudo in controller.clients and target_pseudo != pseudo:
                    controller.start_game(pseudo, target_pseudo)
                    controller.broadcast_message(
                        f"[SERVER] {pseudo} challenges {target_pseudo} to a game!",
                    )
                else:
                    controller.send_message(
                        "Player not available.\n",
                        "SERVER",
                        pseudo,
                        is_server_message=True,
                    )

            elif cmd == "move":
                if len(args) != 1:
                    controller.send_message(
                        "Usage: /move <pit_index>\n",
                        "SERVER",
                        pseudo,
                        is_server_message=True,
                    )
                    continue
                try:
                    pit_index = int(args[0])
                    controller.handle_game_action(pseudo, pit_index)
                except ValueError:
                    controller.send_message(
                        "Please enter a valid pit number.\n",
                        "SERVER",
                        pseudo,
                        is_server_message=True,
                    )

            else:
                controller.send_message(
                    "Unknown command. Type /help for available commands.\n",
                    "SERVER",
                    pseudo,
                    is_server_message=True,
                )

        # Regular messages
        else:
            controller.broadcast_message(msg, pseudo)

    if pseudo:
        with controller.lock:
            if pseudo in controller.clients:
                del controller.clients[pseudo]
        conn.close()


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    controller = GameController()
    print(f"Server listening on {HOST}:{PORT}")

    try:
        while True:
            conn, addr = server.accept()
            controller.accept_connection(conn)
            # Start the heartbeat thread
            threading.Thread(target=heartbeat, args=(conn,), daemon=True).start()
            threading.Thread(
                target=handle_client,
                args=(conn, controller),
            ).start()
    except KeyboardInterrupt:
        print("Server shutting down.")
    finally:
        # Close all client connections gracefully
        with controller.lock:
            for conn in controller.connections:
                # Send a shutdown signal to the socket, to abort any ongoing reads/writes
                conn.shutdown(socket.SHUT_RDWR)
                # Close the socket
                conn.close()

        server.close()
        print("Server closed.")


if __name__ == "__main__":
    main()
