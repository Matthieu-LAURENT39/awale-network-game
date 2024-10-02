import time

PORT: int = 12345
"""Port used by the server."""

TIMEOUT: float = 10.0
"""How long to wait for a response from the server / client."""

LOGGING: bool = False

import json
import socket

from network_data import BaseNetworkData


def log(msg: str):
    """Log a message."""
    if LOGGING:
        print(f"[LOG] {msg}")


def send(sock: socket.socket, data: BaseNetworkData):
    """Send data to a socket."""
    msg = json.dumps(data.serialize())
    msg += "\n"
    log(f"Sending: {msg}")
    sock.sendall(msg.encode())


def heartbeat(
    sock: socket.socket, exit_on_failure: bool = False, interval: float = TIMEOUT / 2
):
    """Thread to send heartbeat to a socket."""
    while True:
        log("Sending heartbeat")
        try:
            sock.sendall(b"hb\n")
            time.sleep(interval)
        except socket.timeout:
            log("Heartbeat timed out - closing connection")
            sock.close()
            if exit_on_failure:
                print("Exiting...")
                exit(1)


buffer = ""


# TODO: make this work with subsequent messages
# probs move to network_data.py and make a send method
def receive(sock: socket.socket, buffer_size: int = 1024) -> dict:
    """Receive data from a socket."""
    global buffer

    sock.settimeout(TIMEOUT)
    try:
        while True:
            data = sock.recv(buffer_size).decode()
            if not data:
                raise ConnectionError("No data received or connection closed")
            buffer += data
            while "\n" in buffer:
                message, _, buffer = buffer.partition("\n")
                # Ignore heartbeat
                if message == "hb":
                    log("Received heartbeat")
                else:
                    log(f"Received: {message}")
                    return json.loads(message)
    except socket.timeout as e:
        raise TimeoutError("Socket timeout") from e
    except socket.error as e:
        raise ConnectionError("Error receiving data from socket") from e
    except json.JSONDecodeError as e:
        raise ValueError("Received data is not valid JSON") from e


def validate_pseudo(pseudo: str) -> bool:
    """Validate a pseudo."""
    return pseudo.isalnum() and len(pseudo) <= 10
