INCLUDES = -I ./include

OBJECTS = server client 
all: ${OBJECTS}
	
server: ./src/server.c
	gcc ${INCLUDES} ./src/server.c -o ./bin/server 

client: ./src/client.c
	gcc ${INCLUDES} ./src/client.c -o ./bin/client 


clean:
	del ./bin*
