all: client

client: client.o commands_file_parse.o getmac.o response_processor.o command_processor.o
	gcc client.o commands_file_parse.o getmac.o response_processor.o  command_processor.o -o client -ljansson

commands_file_parse.o: commands_file_parse.c commands_file_parse.h
	gcc -c commands_file_parse.c 

client.o: client.c commands_file_parse.h response_processor.h
	gcc -c client.c 

getmac.o: getmac.c getmac.h
	gcc -c getmac.c 

response_processor.o: response_processor.c response_processor.h
	gcc -c response_processor.c

command_processor.o: command_processor.c command_processor.h
	gcc -c command_processor.c

clean:
	rm -rf *.o 