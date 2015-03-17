#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ring_interface.h"
#include "defines.h"
#include "common.h"
#include "communication.h"
#include "connections_set.h"

int executeNEW(int id, const char *ip, const char *port, int fd);
int executeCON(int id, const char *ip, const char *port, int fd);

int handleMessage(const char* message, int fd) {
	int error = -1;

	char command[BUFSIZE];	//comando da mensagem
	char arg[5][IPSIZE];	//argumentos da mensagem
	char extra[BUFSIZE];	//utilizado nao para testar se existe um argumento a mais

	//filtrar mensagem
	int argCount = sscanf(message, "%s %s %s %s %s %s %s", command, arg[0], arg[1], arg[2],
															arg[3], arg[4], extra);

	if(strcmp(command, "QRY") == 0 && argCount == 3) {	//QRY message
		putok("mensagem QRY");

		int searcherId = atoi(arg[0]);
		int searchedId = atoi(arg[1]);

		if(searchedId == 0 || searcherId == 0) {
			puterror("handleMessage", "QRY ids invalidos");
			return -1;
		}

		error = 0;

	} else if(strcmp(command, "CON") == 0 && argCount == 4) {
		putok("mensagem de CON");

		int id = atoi(arg[0]);
		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = executeCON(id, arg[1], arg[2], fd)) == -1) {
			puterror("handleMessage", "CON falhou");
		}

	} else if(strcmp(command, "RSP") == 0 && argCount == 6) {
		putok("mensagem RSP");
		error = 0;
	} else if(strcmp(command, "NEW") == 0 && argCount == 4) {
		putok("mensagem de NEW");

		int id = atoi(arg[0]);
		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = executeNEW(id, arg[1], arg[2], fd)) == -1) {
			puterror("handleMessage", "NEW falhou");
		}

	} else if(strcmp(command, "ID") == 0 && argCount == 2) {
		putok("mensagem ID");
		error = 0;
	} else if(strcmp(command, "SUCC") == 0 && argCount == 4) {
		putok("mensagem SUCC");
		error = 0;
	} else if(strcmp(command, "BOOT") == 0 && argCount == 1) {
		putok("mensagem BOOT");
		error = 0;
	} else {
		puterror("handleMessage", "mensagem invalida");
	}

	return error;
}

int executeNEW(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	if(prediNode.fd == -1) {
		//insercao do segundo nó no anel
		putok("insercao do segundo nó no anel");

		//definir novo predi
		prediNode.id = id;
		strcpy(prediNode.ip, ip);
		strcpy(prediNode.port, port);
		prediNode.fd = fd;

		if(succiNode.fd == -1) {

			if( (succiNode.fd = connectToNode(ip, port)) == -1) {
				puterror("executeNEW", "ligacao ao novo succi falhou");
			} else {
				putok("ligacao estabelecida com novo succi");
				//adicionar ao conjunto
				addConnection(succiNode.fd);

				//definir novo succi
				succiNode.id = id;
				strcpy(succiNode.ip, ip);
				strcpy(succiNode.port, port);

				//enviar mensagem de CON ao succi
				if( (error = sendMessageCON(id, ip, port, succiNode.fd)) == -1) {
					puterror("executeNEW", "mensagem de CON nao enviado ao succi");
				}
			}
		} else {
			error = 0;
		}
	} else {
		//enviar mensagem de CON ao succi
		if( (error = sendMessageCON(id, ip, port, prediNode.fd)) == -1) {
			puterror("executeNEW", "mensagem de CON nao enviado ao succi");
		}

		close(prediNode.fd);
		rmConnection(prediNode.fd);

		prediNode.id = id;
		strcpy(prediNode.ip, ip);
		strcpy(prediNode.port, port);
		prediNode.fd = fd;

		error = 0;
	}

	return error;
}

int executeCON(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	if(curNode.id == id) {	//testar se o id do no recebido é o mesmo do no actual
		//existem apenas 2 nós no anel
		//definir predi = succi e predi.fd = %d nova ligacao
		putdebug("2o no a ser inserido: definir predi = succi e predi.fd = %d nova ligacao", fd);
		prediNode.id = succiNode.id;
		strcpy(prediNode.ip, succiNode.ip);
		strcpy(prediNode.port, succiNode.port);
		prediNode.fd = fd;
		error = 0;
	} else {
		//terminar ligacao com succi
		close(succiNode.fd);
		rmConnection(succiNode.fd);

		//ligar ao no recebido
		if( (succiNode.fd = connectToNode(ip, port)) == -1) {
			puterror("executeCON", "ligacao ao novo succi falhou");
		} else {
			putok("ligacao estabelecida com no succi");
			//adicionar ao conjunto
			addConnection(succiNode.fd);

			//definir novo succi
			succiNode.id = id;
			strcpy(succiNode.ip, ip);
			strcpy(succiNode.port, port);

			//enviar NEW ao novo succi
			error = sendMessageNEW(succiNode.fd);
		}
	}

	return error;
}

/*
 *	descricao:	calcula a distancia desde o identificador @srcId até ao @destId
 */
int distance(int srcId, int destId) {
	int dist = INFINITE;

	if(destId < 0 || srcId < 0) {
		dist = INFINITE;
	} else {
		if(destId >= srcId) {
			dist = destId - srcId;
		} else {
			dist = MAXID + destId - srcId;
		}
	}

	return dist;
}
