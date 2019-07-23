all: server

server: server.o data_base.o request_handler.o
	gcc server.o data_base.o request_handler.o -o server -ljansson

server.o: server.c server.h
	gcc -c server.c

data_base.o: data_base.c data_base.h
	gcc -c data_base.c

request_handler.o: request_handler.c request_handler.h data_base.h
	gcc -c request_handler.c

clean:
	rm -rf *.o 
	
