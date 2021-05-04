all:
	gcc init.c -lpthread -o init
	gcc node.c -lpthread -o node
	gcc process.c -lpthread -o process

run:
	./init 100 100
