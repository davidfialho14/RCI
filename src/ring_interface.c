#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ring_interface.h"
#include "defines.h"
#include "common.h"
#include "communication.h"
#include "connections_set.h"
#include "string_operations.h"

int executeNEW(int id, const char *ip, const char *port, int fd);
int executeCON(int id, const char *ip, const char *port, int fd);
int executeID(int id, int fd);

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

		int searcherId;
		int searchedId;

		if(stringToUInt(arg[0], (unsigned int*) &searcherId) == -1 ||
		stringToUInt(arg[1], (unsigned int*) &searchedId) == -1) {
			puterror("handleMessage", "QRY ids da mensagem invalidos");
			return -1;
		}

		//verificar se este nó é o responsavel pelo id procurado
		if(distance(searchedId, curNode.id) < distance(searchedId, prediNode.id)) {
			//nó é responsavel pelo id procurado
			//responder com o próprio IP e porto
			putok("sou o nó responsável: %d %s %s\n", curNode.id, curNode.ip, curNode.port);
			//passar resposta para o predi
			if( (error = sendMessageRSP(prediNode.fd, searcherId, searchedId,
					curNode.id, curNode.ip, curNode.port)) == -1) {
				puterror("executeQRY", "passagem da resposta para o predi");
			}

			error = 0;	//nao ocorreu nenhum erro
		} else {
			int ownerId;
			char ownerIp[BUFSIZE], ownerPort[BUFSIZE];

			if( (error = executeQRY(searcherId, searchedId, &ownerId, ownerIp, ownerPort)) == -1) {
				puterror("handleMessage", "qry falhou");

			} else if(error == 1) {
				//o nó actual foi quem iniciou a procura
				//isto nao é suposto acontecer aqui
				puterror("handleMessage", "Nó responsável incorrecto");
				error = -1;
			} else {
				//mensagem retransmitida
				putok("mensagem retransmitida");
				error = 0;
			}
		}

	} else if(strcmp(command, "CON") == 0 && argCount == 4) {
		putok("mensagem de CON");

		int id;
		if(stringToUInt(arg[0], (unsigned int*) &id) == -1) {
			puterror("handleMessage", "CON id da mensagem invalido");
			return -1;
		}

		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = executeCON(id, arg[1], arg[2], fd)) == -1) {
			puterror("handleMessage", "CON falhou");
		}

	} else if(strcmp(command, "NEW") == 0 && argCount == 4) {
		putok("mensagem de NEW");

		int id;
		if(stringToUInt(arg[0], (unsigned int*) &id) == -1) {
			puterror("handleMessage", "CON id da mensagem invalido");
			return -1;
		}

		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = executeNEW(id, arg[1], arg[2], fd)) == -1) {
			puterror("handleMessage", "NEW falhou");
		}

	} else if(strcmp(command, "ID") == 0 && argCount == 2) {
		putok("mensagem ID");

		int nodeId;
		if(stringToUInt(arg[0], (unsigned int*) &nodeId) == -1) {
			puterror("handleMessage", "ID id da mensagem invalido");
			return -1;
		}

		error = executeID(nodeId, fd);

	} else if(strcmp(command, "BOOT") == 0 && argCount == 1) {
		putok("mensagem BOOT");
		iAmStartNode = TRUE;
		error = 0;

	} else {
		puterror("handleMessage", "mensagem invalida");
	}

	return error;
}

int executeNEW(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	if(prediNode.fd == -1) {	//insercao do segundo nó no anel

		//definir novo predi
		prediNode.id = id;
		strcpy(prediNode.ip, ip);
		strcpy(prediNode.port, port);
		prediNode.fd = fd;

		if(succiNode.fd == -1) {

			if( (succiNode.fd = connectToNode(ip, port)) == -1) {
				puterror("executeNEW", "ligacao ao novo succi falhou");
			} else {
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

		if(id != succiNode.id) {
			//enviar mensagem de CON ao succi
			if( (error = sendMessageCON(id, ip, port, prediNode.fd)) == -1) {
				puterror("executeNEW", "mensagem de CON nao enviado ao succi");
			}
		}

		putok("terminei ligacao %d", prediNode.fd);
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
		prediNode.id = succiNode.id;
		strcpy(prediNode.ip, succiNode.ip);
		strcpy(prediNode.port, succiNode.port);
		prediNode.fd = fd;
		error = 0;

	} else {
		//terminar ligacao com succi
		putok("terminei ligacao %d", succiNode.fd);
		close(succiNode.fd);
		rmConnection(succiNode.fd);

		//ligar ao no recebido
		if( (succiNode.fd = connectToNode(ip, port)) == -1) {
			puterror("executeCON", "ligacao ao novo succi falhou");
		} else {
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
 * 	descricao:	passa a mensagem de QRY ao succi e fica bloqueado à espera da resposta
 * 				quando recebe a resposta caso seja o nó que iniciou a procura retorna
 * 				caso contrario envia a mesma resposta para o seu predi
 * 	retorno:	-1 caso tenha ocorrido algum erro
 * 				0 caso o nó actual não seja quem iniciou a procura
 * 				1 caso o nó actual seja quem iniciou a procura
 */
int executeQRY(int searcherId, int searchedId, int *ownerId, char *ownerIp, char *ownerPort) {
	int error = -1;

	//passar query ao succi
	putdebug("enviar QRY a succi (%d, %s, %s, %d",
			succiNode.id, succiNode.ip, succiNode.port, succiNode.fd);
	if( (error = sendMessageQRY(succiNode.fd, searcherId, searchedId)) == -1) {
		puterror("executeQRY", "mensagem de QRY falhou");

	} else {

		char answer[BUFSIZE];
		bzero(answer, sizeof(answer));
		//esperar resposta
		if( (error = waitForRSP(succiNode.fd, answer, searcherId, searchedId,
				ownerId, ownerIp, ownerPort)) == -1) {
			puterror("executeQRY", "espera por resposta");
		} else {

			if(searcherId == curNode.id) {
				//sou o nó que iniciou a procura
				error = 1;
			} else {
				//passar resposta para o predi
				if( (error = sendMessageRSP(prediNode.fd, searcherId, searchedId,
						*ownerId, ownerIp, ownerPort)) == -1) {
					puterror("executeQRY", "passagem da resposta para o predi");
				}
			}
		}
	}

	return error;
}

int executeID(int id, int fd) {
	int error = -1;

	Node succNode;

	if(distance(id, curNode.id) < distance(id, prediNode.id)) {
		//nó é responsavel pelo id procurado
		succNode.id = curNode.id;
		strcpy(succNode.ip, curNode.ip);
		strcpy(succNode.port, curNode.port);

	} else {

		if( (error = executeQRY(curNode.id, id, &succNode.id, succNode.ip, succNode.port)) != 1) {
			puterror("executeUserCommand", "search falhou");
			return -1;
		}
	}

	//enviar succi encontrado
	if( (error = sendSUCC(fd, &succNode)) == -1) {
		puterror("executeID", "envio de SUCC");
		error = -1;
	} else {

		//fechar ligacao
		putok("terminei ligacao %d", fd);
		close(fd);
		rmConnection(fd);

		error = 0;
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
