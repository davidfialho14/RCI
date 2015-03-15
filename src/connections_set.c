#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "connections_set.h"

fd_set connections;
int connectionCount = 0;	//numero de ligacoes no conjunto
int maxConnection = -1;		//valor do fd, que descreve uma ligacao, mais elevado

void initializeConnectionSet() {
	FD_ZERO(&connections);
}

int getFirstConnection() {
	int connection = -1;

	for(int i = 0; i < maxConnection; ++i) {
		if(FD_ISSET(i, &connections)) {
			connection = i;
			break;
		}
	}

	return connection;
}

int getNextConnection(int curConnection) {
	int connection = -1;

	for(int i = curConnection + 1; i < maxConnection; ++i) {
		if(FD_ISSET(i, &connections)) {
			connection = i;
			break;
		}
	}

	return connection;
}

void addConnection(int connection) {
	FD_SET(connection, &connections);
	if(connection > maxConnection)
		maxConnection = connection;
}

void rmConnection(int connection) {
	FD_CLR(connection, &connections);
	if(connection == maxConnection)
		maxConnection--;
}

int getMaxConnection() {
	return maxConnection;
}

void copySet(fd_set *destinationSet) {
	memcpy(destinationSet, &connections, sizeof(connections));
}
