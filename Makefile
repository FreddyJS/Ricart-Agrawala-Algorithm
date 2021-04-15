all:
	gcc init.c -lpthread -o init
	gcc nodo.c -lpthread -o nodo
	gcc process.c -lpthread -o process

run:
	./init 3 2
