# Định nghĩa các đường dẫn đến thư mục nguồn
APP_DIR = ../app
MODEL_DIR = ../model

all: server client

# Tạo chương trình server và client
server: server.o message.o user.o auction_room.o
	gcc -o server server.o message.o user.o auction_room.o

client: client.o message.o user.o auction_room.o
	gcc -o client client.o message.o user.o auction_room.o

# Biên dịch server.o
server.o: $(APP_DIR)/server.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h
	gcc -c $(APP_DIR)/server.c -o server.o -I$(MODEL_DIR)

# Biên dịch client.o
client.o: $(APP_DIR)/client.c $(MODEL_DIR)/message.h $(MODEL_DIR)/user.h
	gcc -c $(APP_DIR)/client.c -o client.o -I$(MODEL_DIR)

# Biên dịch các thư viện trong model
message.o: $(MODEL_DIR)/message.c $(MODEL_DIR)/message.h
	gcc -c $(MODEL_DIR)/message.c -o message.o

user.o: $(MODEL_DIR)/user.c $(MODEL_DIR)/user.h
	gcc -c $(MODEL_DIR)/user.c -o user.o

auction_room.o: $(MODEL_DIR)/auction_room.c $(MODEL_DIR)/auction_room.h
	gcc -c $(MODEL_DIR)/auction_room.c -o auction_room.o

# Quy tắc dọn dẹp
clean:
	rm -f *.o server client
