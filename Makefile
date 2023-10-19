INCLUDES = -I ./include

OBJECTS = server client 
all: ${OBJECTS}
	
server: ./src/server.c
	gcc ${INCLUDES} ./src/server.c -lpthread -o ./bin/server 

client: ./src/client.c
	gcc ${INCLUDES} ./src/client.c -lpthread -o ./bin/client 

clean:
	rm -f ./bin/*
