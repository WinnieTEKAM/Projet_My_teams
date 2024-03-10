CC = gcc
CFLAGS = -w -Wall -Wextra -std=c11
LDFLAGS =

SERVER_SRC = serveur_my_teams.c
CLIENT_SRC = client_my_teams.c

SERVER_OBJ = $(SERVER_SRC:.c=.o)
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

SERVER_EXEC = server
CLIENT_EXEC = client

.PHONY: all clean fclean re

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)

fclean: clean
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)

re: fclean all

