all:
	gcc init.c -lpthread -o init
	gcc nodo.c -lpthread -o nodo

run:
	./init 5
