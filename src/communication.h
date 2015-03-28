#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <limits.h>
#include "defines.h"

typedef struct Node {

	int id;					//id do no
	char ip[IPSIZE];		//endereco do no - pode ser em nome ou em numero
	char port[PORTSIZE];	//porto do no
	int fd;					//descritor de comunicacao do no
} Node;

extern Node curNode;
extern int curRing;
extern Node prediNode;
extern Node succiNode;
extern char startServerIp[IPSIZE];
extern char startServerPort[PORTSIZE];
extern int startServerFd;
extern int iAmStartNode;

/*****************
 * Inicialização *
 *****************/

/*
	descricao:	interpreta os argumentos de entrada lendo os valores do porto tcp
				usado nas comunicacoes do anel, o ip do servidor de arranque e o porto
				do servidor de arranque. cria o socket de escuta do no para novas ligacoes
				os dados lidos sao armazenados nas variaveis correspondentes:
					curNode.port - porto TCP
					startServerIP - endereco IP do servidor de arranque
					startServerPort - porto do servidor de arranque

	returns:	0 em caso de sucesso e um valor menor do que zero em caso de erro
	erros:		EARGS - os argumentos de entrada nao têm o formato correcto ou
						estao incompletos
*/
int initializeCommunication(int argc, const char *argv[]);

void closeSockets();

/****************************************
 * Comunicacao com servidor de arranque *
 ****************************************/

/*
	descricao: 	executa um pedido ao servidor de arranque pelo o identificador,
				endereco e porto do no de arranque do anel com id @ringId
	retorno:	retorna por referencia a informacao do no de arranque do anel
				retorna por valor um valor de erro
	erro:		caso o anel nao exista é retornado o valor -1 caso contrario é
				retornado o valor 0
*/
int getStartNode(int ringId, Node* startNode);

/*
	descricao:	regista o nó actual como sendo o nó de arranque do anel @ringId com o
				identificador @nodeId. caso o anel @ringId nao exista é criado um anel @ringId
*/
int registerAsStartingNode(int ringId, const Node *node);

int unregisterRing(int ringId);

/**************************
 * Comunicacao com os nós *
 **************************/

/*
 * 	descricao:	estabelece uma ligacao com um no dado o seu endereco e o seu porto
 * 	argumentos:	nodeAddress - endereco do no
 * 				nodePort - porto do no
 * 	retorno:	descritor do socket criado para a comunicacao é retornado por valor
 */
int connectToNode(const char *nodeAddress, const char *nodePort);

void closeConnection(int *fd);

int readMessage(int fd, char *message, size_t messageSize);

int sendMessage(int fd, const char *message);

int sendMessageNEW(int fd);

int sendMessageCON(int id, const char *ip, const char *port, int fd);

int sendMessageQRY(int fd, int searcherId, int searchedId);

int waitForRSP(int fd, char *answer, int searcherId, int searchedId,
		int *ownerId, char *ownerIp, char *ownerPort);

int sendMessageRSP(int fd, int searcherId, int searchedId, int ownerId,
		const char *ownerIp, const char *ownerPort);

int sendMessageBOOT(int fd);

int sendMessageID(int fd, int nodeId);

int waitForSUCC(int fd, Node *succNode);

int sendSUCC(int fd, const Node *succNode);

int sendMessageEND(int id, const char *ip, const char *port, int fd, int start);

int sendMessageRING(int fd, int ring, int id);

#endif
