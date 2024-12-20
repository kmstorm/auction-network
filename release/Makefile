# Định nghĩa các đường dẫn đến thư mục nguồn
APP_DIR = ../app
MODEL_DIR = ../model

# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -D_XOPEN_SOURCE -I$(MODEL_DIR)

all: server client

# Tạo chương trình server và client
server: server.o message.o user.o auction_room.o item.o countdown_timer.o
	$(CC) -o server server.o message.o user.o auction_room.o item.o countdown_timer.o

client: client.o message.o user.o auction_room.o item.o countdown_timer.o
	$(CC) -o client client.o message.o user.o auction_room.o item.o countdown_timer.o

# Biên dịch server.o
server.o: $(APP_DIR)/server.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h
	$(CC) $(CFLAGS) -c $(APP_DIR)/server.c -o server.o

# Biên dịch client.o
client.o: $(APP_DIR)/client.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h
	$(CC) $(CFLAGS) -c $(APP_DIR)/client.c -o client.o

# Biên dịch các thư viện trong model
message.o: $(MODEL_DIR)/message.c $(MODEL_DIR)/message.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/message.c -o message.o

user.o: $(MODEL_DIR)/user.c $(MODEL_DIR)/user.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/user.c -o user.o

auction_room.o: $(MODEL_DIR)/auction_room.c $(MODEL_DIR)/auction_room.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/auction_room.c -o auction_room.o

item.o: $(MODEL_DIR)/item.c $(MODEL_DIR)/item.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/item.c -o item.o

countdown_timer.o: $(MODEL_DIR)/countdown_timer.c $(MODEL_DIR)/countdown_timer.h
	$(CC) $(CFLAGS) -c $(MODEL_DIR)/countdown_timer.c -o countdown_timer.o

# Clean
clean:
	rm -f *.o server client
