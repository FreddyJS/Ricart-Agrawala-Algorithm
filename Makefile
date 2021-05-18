
all:
	gcc init.c -lpthread -o init
	gcc node.c -lpthread -o node
	gcc process.c -lpthread -o process

run: all
	./init 5 5

bigrun: all
	./init 100 100

clean:
	@make -C prueba0 clean
	@make -C prueba1 clean
	@make -C prueba2 clean
	@make -C prueba3 clean
	@make -C prueba4 clean
	@rm init
	@rm node
	@rm process
