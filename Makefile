INCLUDES = -I ./include

OBJECTS = server client 
all: ${OBJECTS}
	
server: ./src/server.c
	gcc ${INCLUDES} ./src/server.c -o ./bin/server -lsqlite3  

client: ./src/client.c
	gcc ${INCLUDES} ./src/client.c -o ./bin/client -lsqlite3  

clean:
	rm -f ./bin/*
