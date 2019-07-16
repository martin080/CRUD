all: client

client: client.o commands_file_parse.o getmac.o
	gcc client.o commands_file_parse.o -o client -ljansson

commands_file_parse.o: commands_file_parse.c commands_file_parse.h
	gcc -c commands_file_parse.c 

client.o: client.c commands_file_parse.h
	gcc -c client.c 

getmac.o: getmac.c getmac.h
	gcc -c getmac.c 

clean:
	rm -rf *.o 