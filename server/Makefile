all: server

server: server.o data_base.o
	gcc server.o data_base.o -o server -ljansson

server.o: server.c 
	gcc -c server.c

data_base.o: data_base.c
	gcc -c data_base.c

clean:
	rm -rf *.o 
	
