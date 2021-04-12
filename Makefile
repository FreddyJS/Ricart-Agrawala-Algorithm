all:
	gcc init.c -o init
	gcc nodo.c -o nodo

run:
	./init 4