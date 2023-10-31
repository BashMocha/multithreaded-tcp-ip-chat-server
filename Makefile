INCLUDES = -I ./include

OBJECTS = server client 
all: ${OBJECTS}
	
server: ./src/server.c
	gcc ${INCLUDES} ./src/server.c ./src/sqlite3.c -o ./bin/server 

client: ./src/client.c
	gcc ${INCLUDES} ./src/client.c ./src/sqlite3.c -o ./bin/client 

clean:
	rm -f ./bin/*
