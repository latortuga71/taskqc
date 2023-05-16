all:
	gcc client.c -o client
	gcc server.c -lpthread -o server
