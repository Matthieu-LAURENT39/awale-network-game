# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS =

# Source files
COMMON_SRCS = common.c
GAME_SRCS = game.c
COLOR_SRCS = color.c
USER_SRCS = user.c
SERVER_SRCS = server.c $(COMMON_SRCS) $(GAME_SRCS) $(COLOR_SRCS) $(USER_SRCS)
CLIENT_SRCS = client.c $(COMMON_SRCS) $(COLOR_SRCS)

# Object files
COMMON_OBJS = $(COMMON_SRCS:.c=.o)
GAME_OBJS = $(GAME_SRCS:.c=.o)
COLOR_OBJS = $(COLOR_SRCS:.c=.o)
USER_OBJS = $(USER_SRCS:.c=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Executables
SERVER_EXEC = server
CLIENT_EXEC = client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Server executable
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Client executable
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Generic rule for building objects
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean the build
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_EXEC) $(CLIENT_EXEC)

# Run server
run-server: $(SERVER_EXEC)
	./$(SERVER_EXEC)

# Run client
run-client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC)

# Phony targets
.PHONY: all clean run-server run-client
