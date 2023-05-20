all:
	gcc client.c -o client
	gcc server.c -lpthread -o server
	gcc broker.c -lpthread -o broker
	gcc worker.c -lpthread -o worker
