"""
Handles the data sent between the server and the client.

This defines the data types that can be sent between the server and the client as typed dicts, to
ensure updates to the data structure are reflected in the code in both the server and the client.

We could make our own data format, but json is basically as good as it gets, and an industry standard.

A client may only ever send ClientMessageData, all other data types are sent by the server.
"""

from dataclasses import dataclass
from typing import ClassVar, Self


def guess_type_and_deserialize(data: dict) -> Self:
    """Deserialize the data from JSON and guess the type."""
    data_type = data.get("type")
    if data_type is None:
        raise ValueError("Data type not found in message.")

    data_class = BaseNetworkData.DATA_TYPES.get(data_type)
    if data_class is None:
        raise ValueError(f"Data type {data_type} not recognized.")

    return data_class.deserialize(data)


@dataclass
class BaseNetworkData:
    """Type of message sent by the server."""

    DATA_TYPES: ClassVar[dict[str, type["BaseNetworkData"]]] = {}

    def __init_subclass__(cls, /, data_type: str) -> None:
        super().__init_subclass__()
        if data_type is None:
            raise ValueError("Network data class initialized without a type.")

        # Make sure we're not already defining the type attribute
        assert not hasattr(cls, "_data_type"), "Data type attribute already defined."
        assert (
            data_type not in cls.DATA_TYPES
        ), f"Data type {data_type} already defined."

        cls.DATA_TYPES[data_type] = cls
        cls._data_type = data_type

    def serialize(self) -> dict:
        """Serialize the data."""
        data = self.__dict__.copy()
        return {"type": self._data_type, **data}

    # TODO: Can probably avoid calling json.loads twice
    @classmethod
    def deserialize(cls, data: dict) -> Self:
        """Deserialize the data."""
        if cls is BaseNetworkData:
            raise NotImplementedError("BaseNetworkData cannot be deserialized.")

        data_type = data.pop("type")
        if data_type != cls._data_type:
            raise ValueError(
                f"Data type {data_type} does not match expected {cls._data_type}."
            )

        return cls(**data)


@dataclass
class ConnectionData(BaseNetworkData, data_type="con"):
    """Data sent by the client when first connecting to the server."""

    pseudo: str
    """Pseudo chosen by the client."""

    # Simulating a little telemetry data for fun
    platform: str
    """Operating system of the client."""


@dataclass
class ConnectionResponse(BaseNetworkData, data_type="con_res"):
    """Response sent by the server after receiving connection data."""

    success: bool
    """Whether the connection was successful."""

    message: str | None
    """Message to display to the client."""


@dataclass
class MessageData(BaseNetworkData, data_type="msg"):
    """A message to display on the client."""

    username: str
    """The username of the sender."""

    message: str
    """The message to display."""

    is_server_message: bool = False
    """Whether the message is from the server."""


@dataclass
class ClientMessageData(BaseNetworkData, data_type="cli_msg"):
    """A message to send to the server."""

    message: str
    """The message to send."""
