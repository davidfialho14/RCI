#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user_interface.h"
#include "defines.h"
#include "communication.h"
#include "common.h"
#include "connections_set.h"
#include "ring_interface.h"

extern int curRing;

int executeDebugJoin(int ring, int nodeId,
		int succiId, const char *succiAddress, const char *succiPort);
int executeUserCommand(const char *input) {
	int error = -1;

	//interpretar o comando
	char command[BUFSIZE];			//comando introduzido
	int ring = -1, nodeId = -1;		//anel pretendido, id prentedido
	int succiId = -1;				//id do succi
	char succiAddress[IPSIZE];		//endereco ip do succi; tem que ser converitdo para inteiro
	char succiPort[PORTSIZE];		//porto do succi
	char extra[BUFSIZE];			//utilizado nao para testar se existo um argumento a mais

	//filtrar linha introduzida
	int argCount = sscanf(input, "%s %d %d %d %s %s %s", command, &ring, &nodeId, &succiId,
			succiAddress, succiPort, extra);
	putdebug("input do utilizador: %s", input);
	
	if(strcmp(command, "join") == 0) { 			//comando join?
		//comando join lido

		//testar se o no ja esta registado
		if(curRing != -1) {
			printf("o no ja esta registado no anel %d\n", curRing);
			return -1;	//retornar erro
		}

		//podemos receber mais 2 argumentos ou mais 5 argumentos
		if(argCount == 3) {
			//tratar join com pesquisa
			putok("comando join com pesquisa: %s %d %d", command, ring, nodeId);

			error = 0;
		} else if(argCount == 6) {
			//tratar join sem pesquisa
			putok("comando join sem pesquisa: %s %d %d %d %s %s", command, ring, nodeId,
					succiId, succiAddress, succiPort);

			if( (error = executeDebugJoin(ring, nodeId, succiId, succiAddress, succiPort)) == -1) {
				puterror("executeUserCommand", "debug join falhou");
			}

		} else {
			putwarning("comando join invalido");
		}

	} else if(strcmp(command, "leave") == 0 && argCount == 1) {	//comando leave?
		//o comando leave nao tem argumentos
		//tratar comando
		putok("comando leave");

	} else if(strcmp(command, "show") == 0 && argCount == 1) {	//comando show?
		//o comando show nao tem argumentos
		//tratar comando
		putok("comando show");

	} else if(strcmp(command, "search") == 0 && argCount == 2) {	//comando search?
		//o comando search aceita apenas 1 argumento
		//tratar comando
		int searchedId = ring;
		putok("comando search: %d", searchedId);

		//testar se o no ja esta registado
		if(curRing == -1) {
			printf("o anel nao esta registado em nenhum anel\n");
			return -1;
		}

		if(distance(searchedId, curNode.id) < distance(searchedId, prediNode.id)) {
			//nó é responsavel pelo id procurado
			//responder com o próprio IP e porto
			printf("Nó responsável: %d %s %s\n", curNode.id, curNode.ip, curNode.port);

			error = 0;	//nao ocorreu nenhum erro
		} else {
			int ownerId;
			char ownerIp[BUFSIZE], ownerPort[BUFSIZE];

			if( (error = executeQRY(curNode.id, searchedId, &ownerId, ownerIp, ownerPort)) == -1) {
				puterror("executeUserCommand", "search falhou");
			} else if(error == 1) {
				//o nó actual foi quem iniciou a procura
				printf("Nó responsavel por %d:\n", searchedId);
				printf("\tid: %d ip: %s porto: %s\n", ownerId, ownerIp, ownerPort);
				error = 0;
			} else {
				//mensagem retransmitida
				//isto nao é suposto acontecer aqui
				puterror("executeUserCommand", "mensagem nao devia ter sido retransmitida");
				error = -1;
			}
		}

	} else if(strcmp(command, "exit") == 0 && argCount == 1) {	//comando exit?
		//o comando exit nao tem argumentos
		putok("comando exit");
		error = 1;	//indica que se pretende sair do programa

	} else {
		error = -1;
	}

	return error;
}

/*
 * descricao:	executa um debug join
 * argumentos:	ring - anel onde fazer join
 * 				nodeId - id que o no pretende ter
 * 				succiId - id do no succi a que o no se pretende ligar
 * 				succiAddress - endereco do succi
 * 				succiPort - porto do succi
 * retorno:		retorna 0 em caso de sucesso e -1 em caso de erro
 */
int executeDebugJoin(int ring, int nodeId,
		int succiId, const char *succiAddress, const char *succiPort) {
	int error = -1;

	if(curRing != -1) {
		putwarning("o no ja esta registado no anel %d", curRing);
		return -1;
	}

	Node startNode;		//vai ser ingorado
	int errorCode = getStartNode(ring, &startNode);
	if(errorCode == 0) {			//anel esta vazio

		Node node;
		node.id = nodeId;
		strcpy(node.ip, curNode.ip);
		strcpy(node.port, curNode.port);

		//registar nó como nó de arranque do anel
		if(registerAsStartingNode(ring, &node) == -1) {
			puterror("executeDebugJoin", "registo de no de arranque falhou");
		} else {
			putok("registado no anel %d com id %d", ring, nodeId);
			//actualizar informacoes do nó tendo em conta que é o unico nó no anel
			curRing = ring;						//definir anel do nó
			succiNode.id = succiNode.fd = -1;	//succi ainda nao definido
			curNode.id = nodeId;				//definir id do nó
			prediNode.id = prediNode.fd = -1;	//predi ainda nao definido
			error = 0;
		}

	} else  if(errorCode == 1){ 	//o anel nao esta vazio

		//ligar ao succi introduzido
		int succiFd = -1;
		if( (succiFd = connectToNode(succiAddress, succiPort)) == -1) {
			puterror("executeDebugJoin", "tentativa de ligacao com succi falhou");

		} else {
			putok("executeDebugJoin", "ligacao estabelecida com no %d %s %s",
					nodeId, succiAddress, succiPort);
			addConnection(succiFd);
			//definir no actual
			curNode.id = nodeId;
			curRing = ring;

			//enviar informacoes do proprio no
			if( (error = sendMessageNEW(succiFd)) == -1) {
				puterror("executeDebugJoin", "envio de mensagem NEW para %d %s",
						succiId, succiPort);
			} else {
				//definir succi introduzido como succi do no
				succiNode.id = succiId;
				strcpy(succiNode.ip, succiAddress);
				strcpy(succiNode.port, succiPort);
				succiNode.fd = succiFd;

				putok("definido no succi %d %s %s %d",
						succiNode.id, succiNode.ip, succiNode.port, succiNode.fd);
			}
		}

	} else {
		puterror("executeDebugJoin", "getStartNode falhou");
	}

	return error;
}
