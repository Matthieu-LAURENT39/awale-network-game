"""Client for the Awale game."""

import socket
import threading
from platform import platform

from common import PORT, heartbeat, receive, send
from network_data import (
    ClientMessageData,
    ConnectionData,
    ConnectionResponse,
    MessageData,
    guess_type_and_deserialize,
)

HOST = "localhost"


def receive_messages(sock: socket.socket):
    """Receive messages from the server."""
    while True:
        try:
            data = receive(sock)

            msg = guess_type_and_deserialize(data)
            if isinstance(msg, MessageData):
                if msg.is_server_message:
                    print(f"[{msg.username}] {msg.message}")
                else:
                    print(f"({msg.username}) {msg.message}")
            else:
                print(f"Received data of unknown type {type(msg)}: {msg}")
                print("Please report this to a developer.")

        except TimeoutError:
            print("Connection to the server timed out.")
            print("Exiting...")
            break


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((HOST, PORT))
    # Start the heartbeat thread
    threading.Thread(target=heartbeat, args=(sock, True), daemon=True).start()

    # Send the authentication data
    while True:
        pseudo = input("Choose a pseudo: ")
        conn_data = ConnectionData(pseudo=pseudo, platform=platform())
        send(sock, conn_data)
        raw_response = receive(sock)
        response = ConnectionResponse.deserialize(raw_response)

        if response.message:
            print(response.message)
        if response.success:
            break

    threading.Thread(target=receive_messages, args=(sock,), daemon=True).start()
    try:
        while True:
            message = input()
            if message:
                send(sock, ClientMessageData(message=message))
    except KeyboardInterrupt:
        print("Disconnected from server.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
