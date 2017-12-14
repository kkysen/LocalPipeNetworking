CC = gcc -ggdb -std=c99 -Wall -Werror -O3
SERVER_OUT = server
CLIENT_OUT = client

util/utils.o:
	$(CC) -c util/utils.c -o util/utils.o

util/string_builder.o:
	$(CC) -c util/string_builder.c -o util/string_builder.o

util/stacktrace.o:
	$(CC) -c util/stacktrace.c -o util/stacktrace.o

util/random.o:
	$(CC) -c util/random.c -o util/random.o

pipe_networking.o:
	$(CC) -c pipe_networking.c

UTILS = pipe_networking.o util/random.o util/stacktrace.o util/string_builder.o util/utils.o

basic_server.o:
	$(CC) -c basic_server.c

client.o:
	$(CC) -c client.c

$(SERVER_OUT): basic_server.o $(UTILS)
	$(CC) -o $(SERVER_OUT) basic_server.o $(UTILS)

$(CLIENT_OUT): client.o $(UTILS)
	$(CC) -o $(CLIENT_OUT) client.o $(UTILS)

all: clean $(SERVER_OUT) $(CLIENT_OUT)

clean:
	touch dummy.o
	find . -name '*.o' -delete
	rm -f $(OUT)

install: clean all

run: install
	./$(SERVER_OUT)

rerun: all
	./$(SERVER_OUT)

valgrind: clean all
	valgrind -v --leak-check=full ./$(OUT)

