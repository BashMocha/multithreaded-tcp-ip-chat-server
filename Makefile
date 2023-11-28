INCLUDES = -I ./include

OBJECTS = server client 
all: ${OBJECTS}
	
server: ./src/server.c
	gcc ${INCLUDES} ./src/server.c -o ./bin/server -lm -lsqlite3  

client: ./src/client.c
	gcc ${INCLUDES} ./src/client.c -o ./bin/client -lm -lsqlite3  

clean:
	rm -f ./bin/*
