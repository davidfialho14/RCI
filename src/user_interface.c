#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "user_interface.h"
#include "defines.h"
#include "communication.h"
#include "common.h"
#include "connections_set.h"
#include "ring_interface.h"
#include "string_operations.h"

extern int curRing;
extern int iAmStartNode;

int join(int ring, int nodeId, int succiId, const char *succiAddress, const char *succiPort);
int leave();

int executeUserCommand(const char *input) {
	int error = -1;

	//interpretar o comando
	char command[BUFSIZE/4];			//comando introduzido
	char arg[6][BUFSIZE/4];			//argumentos lidos

	//filtrar linha introduzida
	putdebug("input do utilizador: %s", input);
	int argCount = sscanf(input, "%s %s %s %s %s %s %s",
												command, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);

	if(strcmp(command, "join") == 0) { 			//comando join?
		//comando join lido

		//testar se o nó já pertence a um anel
		if(curRing != -1) {
			putmessage("o nó já está registado no anel %d\n", curRing);
			return -1;
		}

		int ring = -1; 		//args[0]
		int nodeId = -1;	//args[1]

		if(stringToUInt(arg[0], (unsigned int*) &ring) == -1 ||
		stringToUInt(arg[1], (unsigned int*) &nodeId) == -1) {
			putmessage("comando join inválido: argumentos inválidos\n");
			return -1;	//retornar erro
		}

		//testar o valor do identificador pretendido
		if(nodeId > MAXID) {
			putmessage("o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		//podemos receber mais 2 argumentos ou mais 5 argumentos
		if(argCount == 3) {
			//tratar join com pesquisa
			putok("comando join com pesquisa: %s %d %d", command, ring, nodeId);

			if( (error = join(ring, nodeId, -1, NULL, NULL)) == -1) {
				putdebug("executeUserCommand", "join falhou");
			}

		} else if(argCount == 6) {
			//tratar join sem pesquisa
			int succiId	= -1;
			if(stringToUInt(arg[2], (unsigned int*) &succiId) == -1) {
				putmessage("comando join inválido: argumentos inválidos\n");
				return -1;
			}

			char *succiAddress = arg[3];
			char *succiPort = arg[4];

			putok("comando join sem pesquisa: %s %d %d %d %s %s", command, ring, nodeId,
					succiId, succiAddress, succiPort);

			if( (error = join(ring, nodeId, succiId, succiAddress, succiPort)) == -1) {
				putdebug("executeUserCommand", "debug join falhou");
			}

		} else {
			putmessage("comando join inválido: argumentos inválidos\n");
		}

	} else if(strcmp(command, "leave") == 0 && argCount == 1) {	//comando leave?
		//o comando leave nao tem argumentos
		//tratar comando
		putok("comando leave");

		//testar se o nó está registado num anel
		if(curRing == -1) {
			putmessage("nó não se encontra registado em nenhum anel\n");
			return -1;
		}

		if( (error = leave()) == -1) {
			putdebug("executeUserCommand", "leave falhou");
		}

	} else if(strcmp(command, "show") == 0 && argCount == 1) {	//comando show?
		//o comando show nao tem argumentos
		//tratar comando
		putok("comando show");

		if(curRing == -1) {
			putmessage("nó não pertence a nenhum anel\n");
		} else {
			putmessage("Anel: %d\n", curRing);
			putmessage("Id: %d\n", curNode.id);

			putmessage("Succi Id: ");
			if(succiNode.fd == -1) {
					putmessage("não definido\n");
			} else {
				putmessage("%d\n", succiNode.id);
			}

			putmessage("Predi Id: ");
			if(succiNode.fd == -1) {
					putmessage("não definido\n");
			} else {
				putmessage("%d\n", prediNode.id);
			}
		}

		error = 0;

	} else if(strcmp(command, "search") == 0 && argCount == 2) {	//comando search?
		//o comando search aceita apenas 1 argumento
		//tratar comando

		int searchedId	= -1;
		if(stringToUInt(arg[0], (unsigned int*) &searchedId) == -1) {
			putmessage("comando search inválido: argumentos inválidos\n");
			return -1;
		}

		//testar se o no ja esta registado
		if(curRing == -1) {
			putmessage("o anel não está registado em nenhum aneis\n");
			return -1;
		}

		//testar o valor do identificador pretendido
		if(searchedId > MAXID) {
			putmessage("o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		putdebug("executeUserCommand", "comando search: %d\n", searchedId);

		//verificar se o nó que efectuou a pesquisa é o nó responsável
		if(distance(searchedId, curNode.id) < distance(searchedId, prediNode.id)) {
			//nó é responsavel pelo id procurado
			//responder com o próprio IP e porto
			putmessage("nó responsavel por %d:\n", searchedId);
			putmessage("\tid: %d ip: %s porto: %s\n", curNode.id, curNode.ip, curNode.port);
			error = 0;	//nao ocorreu nenhum erro
		} else {
			int ownerId;
			char ownerIp[BUFSIZE], ownerPort[BUFSIZE];

			if( (error = handleQRY(curNode.id, searchedId, &ownerId, ownerIp, ownerPort)) == -1) {
				putdebug("executeUserCommand", "pesquisa pelo id %d falhou", searchedId);
			} else if(error == 1) {
				//o nó actual foi quem iniciou a procura
				putmessage("nó responsavel por %d:\n", searchedId);
				putmessage("\tid: %d ip: %s porto: %s\n", ownerId, ownerIp, ownerPort);
				error = 0;
			} else {
				//mensagem retransmitida
				//isto nao é suposto acontecer aqui
				putdebug("executeUserCommand", "mensagem não devia ter sido retransmitida");
				error = -1;
			}
		}

	} else if(strcmp(command, "exit") == 0 && argCount == 1) {	//comando exit?
		//o comando exit nao tem argumentos
		putok("comando exit");

		//sair de forma limpa
		if(curRing != -1) {
			leave();
		}

		error = 1;	//indica que se pretende sair do programa

	} else {
		putmessage("comando inválido\n");
		error = -1;
	}

	return error;
}

/*
 * descricao:	executa um join de um nó num determinado anel
 * argumentos:
 *	ring - anel onde fazer join
 * 	nodeId - id que o no pretende ter
 * 	succiId - id do no succi a que o no se pretende ligar
 * 	succiAddress - endereco do succi
 * 	succiPort - porto do succi
 * retorno:		retorna 0 em caso de sucesso e -1 em caso de erro
 */
int insertNode(int ring, int nodeId, int succiId,
	const char *succiAddress, const char *succiPort);
int join(int ring, int nodeId, int succiId, const char *succiAddress, const char *succiPort) {
	int error = -1;

	Node startNode;		//vai ser ingorado
	int errorCode = getStartNode(ring, &startNode);
	if(errorCode == 0) {			//anel esta vazio

		Node node;
		node.id = nodeId;
		strcpy(node.ip, curNode.ip);
		strcpy(node.port, curNode.port);

		//registar nó como nó de arranque do anel
		iAmStartNode = TRUE;
		if(registerAsStartingNode(ring, &node) == -1) {
			putdebug("executeDebugJoin", "registo como nó de arranque falhou");
		} else {
			putmessage("registado no anel %d com id %d\n", ring, nodeId);
			//actualizar informacoes do nó tendo em conta que é o unico nó no anel
			curRing = ring;						//definir anel do nó
			succiNode.id = succiNode.fd = -1;	//succi ainda nao definido
			curNode.id = nodeId;				//definir id do nó
			prediNode.id = prediNode.fd = -1;	//predi ainda nao definido
			error = 0;
		}

	} else if(errorCode == 1){ 	//o anel nao esta vazio

		if(succiId == -1 || succiAddress == NULL || succiPort == NULL) {
			//join com pesquisa

			//estabelecer uma ligacao com o nó de arranque
			if( (startNode.fd = connectToNode(startNode.ip, startNode.port)) == -1) {
				putdebug("executeJoin", "ligacao com nó de arranque falhou");
				puterror("ligação ao nó de arranque do anel falhou");
			} else {

				//pedir ao nó de arranque as informacoes do seu succi no anel
				if( (error = sendMessageID(startNode.fd, nodeId)) == -1) {
					putdebug("executeJoin", "pedido do endereço do succi ao nó de arranque falhou");
					puterror("comunicação com o nó de arranque do anel falhou");
				} else {

					//esperar pela resposta do nó de arranque
					Node succ;
					if( (error = waitForSUCC(startNode.fd, &succ)) == -1) {
						putdebug("executeJoin", "espera pela resposta do servidor de arranque");
						puterror("comunicação com o nó de arranque do anel falhou");
					} else {

						//fechar ligacao com succi
						putok("terminei ligacao %d", startNode.fd);
						close(startNode.fd);
						rmConnection(startNode.fd);
						startNode.fd = -1;

						//testar se o id do succi retornado pela pesquisa é igual ao id pretendido
						if(succ.id == nodeId) {
							putmessage("o identificador %d já existe no anel, escolha outro por favor\n", nodeId);
							error = -1;
						} else {
							//inserir nó no anel
							error = insertNode(ring, nodeId, succ.id, succ.ip, succ.port);
						}

					}
				}
			}

		} else {
			//inserir nó no anel com endereco dado
			error = insertNode(ring, nodeId, succiId, succiAddress, succiPort);
		}

	} else {
		puterror("comunicação com o servidor de arranque falhou");
		putdebug("executeDebugJoin", "pedido de nó de arranque falhou");
	}

	return error;
}

int leave() {
	int error = -1;

	//testar se o nó é o único no anel
	if(succiNode.fd == -1 && prediNode.fd == -1) {
		error = unregisterRing(curRing);
	} else {

		if(iAmStartNode) {	//testar se o nó actual é o nó de arranque do anel

			//registar succi como no de arranque
			if(registerAsStartingNode(curRing, &succiNode) == -1) {
				putdebug("executeDebugJoin", "registo como nó de arranque falhou");
				return -1;
			}

			//enviar BOOT a succi
			if(sendMessageBOOT(succiNode.fd) == -1) {
				putdebug("executeDebugJoin", "envio de mensagem de BOOT para o succi falhou");
				return -1;
			}
		}

		//fechar ligacao com succi
		putok("terminei ligacao com succi %d", succiNode.fd);
		close(succiNode.fd);
		rmConnection(succiNode.fd);
		succiNode.fd = -1;
		succiNode.id = -1;

		//enviar informacoes do succi a predi
		if( (error = sendMessageCON(succiNode.id, succiNode.ip, succiNode.port, prediNode.fd)) == -1) {
			putdebug("leave", "mensagem de CON a predi");
			return -1;
		}

		putok("terminei ligacao com predi %d", prediNode.fd);
		close(prediNode.fd);
		rmConnection(prediNode.fd);
		prediNode.fd = -1;
		prediNode.id = -1;
	}

	//indicar que o nó nao tem anel definido
	curRing = -1;

	return error;
}

int insertNode(int ring, int nodeId, int succiId, const char *succiAddress, const char *succiPort) {
	int error = -1;

	//ligar ao succi introduzido
	int succiFd = -1;
	if( (succiFd = connectToNode(succiAddress, succiPort)) == -1) {
		putdebug("executeDebugJoin", "tentativa de ligação com succi (%s %s) falhou", succiAddress, succiPort);
	} else {
		//adionar ligacao ao conjunto
		addConnection(succiFd);

		//definir nó com id e ring indicados
		curNode.id = nodeId;
		curRing = ring;

		//enviar informacoes do proprio nó
		if( (error = sendMessageNEW(succiFd)) == -1) {
			putdebug("executeDebugJoin", "envio de mensagem NEW para %d %s falhou", succiId, succiPort);
		} else {

			//definir succi introduzido como succi do nó
			succiNode.id = succiId;
			strcpy(succiNode.ip, succiAddress);
			strcpy(succiNode.port, succiPort);
			succiNode.fd = succiFd;

			putmessage("novo nó inserido no anel %d com succi %d %s %s\n",
				curRing, succiNode.id, succiNode.ip, succiNode.port);
		}
	}

	return error;
}
