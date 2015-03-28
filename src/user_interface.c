#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "user_interface.h"
#include "defines.h"
#include "communication.h"
#include "connections_set.h"
#include "error.h"
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

	if(argCount == -1) {
		//o utilizador apenas carregou no enter - ignorar
		return -1;
	}

	if(strcmp(command, "join") == 0) { 			//comando join?
		//comando join lido

		//testar se o nó já pertence a um anel
		if(curRing != -1) {
			puterror("o nó já está registado no anel %d\n", curRing);
			return -1;
		}

		int ring = -1; 		//args[0]
		int nodeId = -1;	//args[1]

		if(stringToUInt(arg[0], (unsigned int*) &ring) == -1 ||
		stringToUInt(arg[1], (unsigned int*) &nodeId) == -1) {
			puterror("comando join inválido: argumentos inválidos\n");
			return -1;	//retornar erro
		}

		//testar o valor do identificador pretendido
		if(nodeId > MAXID) {
			puterror("o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		//podemos receber mais 2 argumentos ou mais 5 argumentos
		if(argCount == 3) {
			//tratar join com pesquisa
			putdebug("comando join com pesquisa: %s %d %d", command, ring, nodeId);

			if( (error = join(ring, nodeId, -1, NULL, NULL)) == -1) {
				putdebugError("executeUserCommand", "join falhou");
			}

		} else if(argCount == 6) {
			//tratar join sem pesquisa
			int succiId	= -1;
			if(stringToUInt(arg[2], (unsigned int*) &succiId) == -1) {
				puterror("comando join inválido: argumentos inválidos\n");
				return -1;
			}

			char *succiAddress = arg[3];
			char *succiPort = arg[4];

			putdebug("comando join sem pesquisa: %s %d %d %d %s %s", command, ring, nodeId,
					succiId, succiAddress, succiPort);

			if( (error = join(ring, nodeId, succiId, succiAddress, succiPort)) == -1) {
				putdebugError("executeUserCommand", "debug join falhou");
			}

		} else {
			puterror("comando join inválido: argumentos inválidos\n");
		}

	} else if(strcmp(command, "show") == 0 && argCount == 1) {	//comando show?
		//o comando show nao tem argumentos
		//tratar comando
		putdebug("comando show");

		if(curRing == -1) {
			putmessage("nó não pertence a nenhum anel\n");

		} else {
			printf("Anel: %d\n", curRing);
			printf("Id: %d\n", curNode.id);

			printf("Succi Id: ");
			if(succiNode.fd == -1) {
				printf("não definido\n");
			} else {
				printf("%d\n", succiNode.id);
			}

			printf("Predi Id: ");
			if(prediNode.fd == -1) {
				printf("não definido\n");
			} else {
				printf("%d\n", prediNode.id);
			}
		}

		error = 0;

	} else if(strcmp(command, "search") == 0 && argCount == 2) {	//comando search?
		//o comando search aceita apenas 1 argumento
		//tratar comando

		int searchedId	= -1;
		if(stringToUInt(arg[0], (unsigned int*) &searchedId) == -1) {
			puterror("comando search inválido: argumentos inválidos\n");
			return -1;
		}

		//testar se o no ja esta registado
		if(curRing == -1) {
			puterror("o nó não está registado em nenhum anel\n");
			return -1;
		}

		//testar o valor do identificador pretendido
		if(searchedId > MAXID) {
			puterror("o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		putdebug("executeUserCommand", "comando search: %d\n", searchedId);

		//verificar se o nó que efectuou a pesquisa é o nó responsável
		if(distance(searchedId, curNode.id) < distance(searchedId, prediNode.id)) {
			//nó é responsavel pelo id procurado
			//responder com o próprio IP e porto
			printf("nó responsavel por %d:\n", searchedId);
			printf("\tid: %d ip: %s porto: %s\n", curNode.id, curNode.ip, curNode.port);
			error = 0;	//nao ocorreu nenhum erro
		} else {
			int ownerId;
			char ownerIp[BUFSIZE], ownerPort[BUFSIZE];

			if( (error = handleQRY(curNode.id, searchedId, &ownerId, ownerIp, ownerPort)) == -1) {
				putdebugError("executeUserCommand", "pesquisa pelo id %d falhou", searchedId);
				puterror("não foi possível excutar pesquisa\n");
				putmessage("Nota: verifique se existe uma ligação à rede estabelecida\n");

			} else if(error == 1) {
				//o nó actual foi quem iniciou a procura
				printf("nó responsavel por %d:\n", searchedId);
				printf("\tid: %d ip: %s porto: %s\n", ownerId, ownerIp, ownerPort);
				error = 0;
			} else {
				//mensagem retransmitida
				//isto nao é suposto acontecer aqui
				putdebugError("executeUserCommand", "mensagem não devia ter sido retransmitida");
				error = -1;
			}
		}

	} else if(strcmp(command, "leave") == 0 && argCount == 1) {	//comando leave?
		//o comando leave nao tem argumentos
		//tratar comando
		putdebug("comando leave");

		//testar se o nó está registado num anel
		if(curRing == -1) {
			puterror("nó não se encontra registado em nenhum anel\n");
			return -1;
		}

		if( (error = leave()) == -1) {
			putdebug("executeUserCommand", "leave falhou");
		} else {
			putmessage("nó saiu do anel com sucesso\n");
		}

	} else if(strcmp(command, "exit") == 0 && argCount == 1) {	//comando exit?
		//o comando exit nao tem argumentos
		putdebug("comando exit");

		//sair de forma limpa
		if(curRing != -1) {
			leave();
		}

		error = 1;	//indica que se pretende sair do programa

	} else {
		puterror("comando inválido\n");
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

	Node succ;		// informacoes do succi com quem fazer join

	if(succiId == -1 || succiAddress == NULL || succiPort == NULL) {
		// join com pesquisa
		putdebug("join com pesqisa");

		// obter nó de arranque caso o anel já exista
		Node startNode;		//vai ser ingorado
		int errorCode = getStartNode(ring, &startNode);

		if(errorCode == 0) {			//anel esta vazio

			succiNode.id = succiNode.fd = -1;	//succi ainda nao definido
			prediNode.id = prediNode.fd = -1;	//predi ainda nao definido
			curNode.id = nodeId;				//definir id do nó

			//registar nó como nó de arranque do anel
			iAmStartNode = TRUE;
			if(registerAsStartingNode(ring, &curNode) == -1) {
				putdebugError("join", "registo como nó de arranque falhou");
				puterror("tentativa de registo no servidor de arranque falhou\n");
				curNode.id = -1;	//colocar id do nó como não definido
				return -1;

			} else {
				//definir anel do nó
				curRing = ring;
				putmessage("registado no anel %d com id %d\n", curRing, curNode.id);
				//terminado o registo
				return 0;
			}

		} else if(errorCode == 1) { 	//o anel nao esta vazio

			if(strcmp(startNode.ip, curNode.ip) == 0 && strcmp(startNode.port, curNode.port) == 0) {
				putdebugError("join", "servidor de arranque retornou endereco de arranque igual ao do nó");
				puterror("servidor de arranque está desactualizado\n");
				putmessage("exprimente juntar-se a outro anel\n");
				return -1;
			}

			//testar se o id do succi retornado pela pesquisa é igual ao id pretendido
			if(startNode.id == nodeId) {
				puterror("o identificador %d já existe no anel\n" , nodeId);
				putmessage("escolha outro identificador\n");
				return -1;
			}

			//estabelecer uma ligacao com o nó de arranque
			if( (startNode.fd = connectToNode(startNode.ip, startNode.port)) == -1) {
				putdebugError("join", "ligacao com nó de arranque falhou");
				puterror("ligação ao nó de arranque do anel falhou\n");
				putmessage("Nota: é possível que este erro resulte de um servidor desactualizado\n");
				return -1;
			}

			//pedir ao nó de arranque as informacoes do seu succi no anel
			if(sendMessageID(startNode.fd, nodeId) == -1) {
				putdebugError("join", "pedido do endereço do succi ao nó de arranque falhou");
				puterror("comunicação com o nó de arranque do anel falhou\n");
				putmessage("Sugestão: verifique a ligação à rede do anel");
				putmessage("Nota: caso não haja problemas com a ligação isto deve-se provavelmente a um servidor desactualizado\n");
				putmessage("Sugestão: nesse caso escolha outro anel\n");
				closeConnection(&startNode.fd);
				return -1;
			}

			//esperar pela resposta do nó de arranque
			int errorCode = waitForSUCC(startNode.fd, &succ);
			if( errorCode == -1) {
				putdebugError("join", "espera pela resposta do nó de arranque");
				puterror("comunicação com o nó de arranque do anel falhou\n");
				putmessage("Sugestão: verifique a ligação à rede do anel");
				putmessage("Nota: caso não haja problemas com a ligação isto deve-se provavelmente a um servidor desactualizado\n");
				putmessage("Sugestão: nesse caso escolha outro anel\n");
				closeConnection(&startNode.fd);
				return -1;
			} else if(errorCode == -2) {
				puterror("o anel escolhido está ocupado\n");
				putmessage("Sugestão 1: espere 2 segundos e volte a tentar\n");
				putmessage("Sugestão 2: exprimente outro anel\n");
				closeConnection(&startNode.fd);
				return -1;
			}

			//fechar ligacao com succi
			putdebug("terminei ligacao com succi %d", startNode.fd);
			close(startNode.fd);
			rmConnection(startNode.fd);
			startNode.fd = -1;

			//testar se o id do succi retornado pela pesquisa é igual ao id pretendido
			if(succ.id == nodeId) {
				puterror("o identificador %d já existe no anel\n" , nodeId);
				putmessage("escolha outro identificador\n");
				return -1;
			}

		} else {
			puterror("comunicação com o servidor de arranque falhou\n");
			putdebugError("join", "pedido de nó de arranque falhou");
			return -1;
		}

	} else {
		//join sem pesquisa
		putdebug("join sem pesquisa");

		if(strcmp(curNode.ip, succiAddress) == 0 && strcmp(curNode.port, succiPort) == 0) {
			puterror("o endereço do nó corresponde ao endereço indicado para o succi\n");
			putmessage("Sugestão: ligue-se a um nó com um endereço diferente e/ou porto diferentes dos do próprio nó\n");
			return -1;
		}

		if(nodeId == succiId) {
			puterror("o nó e o seu succi não podem ter o mesmo identificador\n");
			putmessage("altere um dos identificadores\n");
			return -1;
		}

		//succ é definido com os valores recebidos
		succ.id = succiId;
		strcpy(succ.ip, succiAddress);
		strcpy(succ.port, succiPort);
	}

	//inserir nó no anel com o succi obtido
	if(insertNode(ring, nodeId, succ.id, succ.ip, succ.port) == -1) {
		puterror("inserção no anel falhou\n");
		return -1;
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
				putdebugError("leave", "tentativa de registo no servidor de arranque falhou");
				return -1;
			}

			//enviar BOOT a succi
			if(sendMessageBOOT(succiNode.fd) == -1) {
				putdebugError("leave", "envio de mensagem de BOOT para o succi falhou");
				return -1;
			}
		}

		//fechar ligacao com succi
		putdebug("terminei ligacao com succi %d", succiNode.fd);
		close(succiNode.fd);
		rmConnection(succiNode.fd);
		succiNode.fd = -1;

		//enviar informacoes do succi a predi
		if( (error = sendMessageCON(succiNode.id, succiNode.ip, succiNode.port, prediNode.fd)) == -1) {
			putdebugError("leave", "mensagem de CON a predi falhou");
			return -1;
		}

		//id so pode ser apagado depois das informacoes do succi serem enviadas a predi
		succiNode.id = -1;

		putdebug("terminei ligacao com predi %d", prediNode.fd);
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
		putdebug("insertNode", "tentativa de ligação com succi (%s %s) falhou", succiAddress, succiPort);
		puterror("não foi possivel ligar ao succi\n");
	} else {
		//adionar ligacao ao conjunto
		addConnection(succiFd);

		//definir nó com id indicado
		curNode.id = nodeId;
		//definir que o nó esta a tentar ser inserido no anel @ring
		insertingInRing = ring;

		//enviar informacoes do proprio nó
		if( (error = sendMessageNEW(succiFd)) == -1) {
			putdebugError("insertNode", "envio de mensagem NEW para %d %s falhou", succiId, succiPort);
		} else {

			//definir succi introduzido como succi do nó
			succiNode.id = succiId;
			strcpy(succiNode.ip, succiAddress);
			strcpy(succiNode.port, succiPort);
			succiNode.fd = succiFd;
		}
	}

	return error;
}
