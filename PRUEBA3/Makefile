
all:
	gcc init.c -lpthread -o init
	gcc node.c -lpthread -o node
	gcc process.c -lpthread -o process

synctime:
	gcc -DSYNCTIME init.c -lpthread -o init
	gcc -DSYNCTIME node.c -lpthread -o node
	gcc -DSYNCTIME process.c -lpthread -o process

run: all
	./init 1 5

bigrun: all
	./init 10 100

clean:
	@rm init
	@rm node
	@rm process
