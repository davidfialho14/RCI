#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ring_interface.h"
#include "defines.h"
#include "communication.h"
#include "connections_set.h"
#include "error.h"
#include "string_operations.h"

int insertingInRing = -1;
fd_set readFds;

int handleNEW(int id, const char *ip, const char *port, int fd);
int handleCON(int id, const char *ip, const char *port, int fd);
int handleID(int id, int fd);
int handleEND(int id, const char *ip, const char *port, int start);
int handleRING(int ring, int id);

int handleMessage(const char* message, int fd) {
	int error = -1;

	char command[BUFSIZE];	//comando da mensagem
	char arg[5][IPSIZE];	//argumentos da mensagem
	char extra[BUFSIZE];	//utilizado nao para testar se existe um argumento a mais

	//filtrar mensagem
	int argCount = sscanf(message, "%s %s %s %s %s %s %s", command, arg[0], arg[1], arg[2],
															arg[3], arg[4], extra);
	//identificar tipo de mensagem

	if(strcmp(command, "CON") == 0 && argCount == 4) {	//mensagem CON
		putdebug("mensagem de CON");

		int id;
		if(stringToUInt(arg[0], (unsigned int*) &id) == -1) {
			putdebugError("handleMessage", "CON id da mensagem inválido");
			return -1;
		}

		//testar o valor do identificador pretendido
		if(id > MAXID) {
			putdebugError("handleMessage", "o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = handleCON(id, arg[1], arg[2], fd)) == -1) {
			putdebugError("handleMessage", "CON falhou");
		}

	} else if(strcmp(command, "NEW") == 0 && argCount == 4) {	//mensagem NEW
		putdebug("mensagem de NEW");

		if(curRing == -1 && insertingInRing == -1) {
			putdebugError("handleMessage", "nó recebeu uma mensagem NEW sem estar inserido em nenhum anel ou a tentar inserir-se");
			return -1;
		}

		int id;
		if(stringToUInt(arg[0], (unsigned int*) &id) == -1) {
			putdebugError("handleMessage", "CON id da mensagem inválido");
			return -1;
		}

		//testar o valor do identificador pretendido
		if(id > MAXID) {
			putdebugError("executeUserCommand", "o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
			return -1;
		}

		//arg[1] - endereco IP
		//arg[2] - porto

		if( (error = handleNEW(id, arg[1], arg[2], fd)) == -1) {
			putdebugError("handleMessage", "NEW falhou");
		}

	} else {

		if(curRing == -1) {
			putdebugError("handleMessage", "nó recebeu uma mensagem TCP não CON sem estar inserido em nenhum anel");
			return -1;
		}

		if(strcmp(command, "QRY") == 0 && argCount == 3) {	//QRY message
			putdebug("mensagem QRY");

			//verificar que foi o predi quem fez a pesquisa
			if(fd != prediNode.fd) {
				putdebugError("handleMessage", "QRY feito sem ser pelo predi");
				return -1;
			}

			int searcherId;
			int searchedId;

			if(stringToUInt(arg[0], (unsigned int*) &searcherId) == -1 ||
			stringToUInt(arg[1], (unsigned int*) &searchedId) == -1) {
				putdebugError("handleMessage", "QRY ids da mensagem inválidos");
				return -1;
			}

			//verificar se este nó é o responsavel pelo id procurado
			if(distance(searchedId, curNode.id) < distance(searchedId, prediNode.id)) {
				//nó é responsavel pelo id procurado
				//responder com o próprio IP e porto
				putdebug("sou o nó responsável por %d: %d %s %s\n", searchedId, curNode.id, curNode.ip, curNode.port);
				//passar resposta para o predi
				if( (error = sendMessageRSP(prediNode.fd, searcherId, searchedId,
						curNode.id, curNode.ip, curNode.port)) == -1) {
						putdebugError("handleQRY", "passagem da resposta para o predi falhada");
				}

				error = 0;	//nao ocorreu nenhum erro
			} else {
				int ownerId;
				char ownerIp[BUFSIZE], ownerPort[BUFSIZE];

				if( (error = handleQRY(searcherId, searchedId, &ownerId, ownerIp, ownerPort)) == -1) {
					putdebugError("handleMessage", "QRY falhou");

				} else if(error == 1) {
					//o nó actual foi quem iniciou a procura
					//isto nao é suposto acontecer aqui
					putdebugError("handleMessage", "nó responsável incorrecto");
					error = -1;
				} else {
					//mensagem retransmitida
					putdebug("mensagem retransmitida");
					error = 0;
				}
			}
		} else if(strcmp(command, "ID") == 0 && argCount == 2) {	//mensagem ID
			putdebug("mensagem ID");

			int nodeId;
			if(stringToUInt(arg[0], (unsigned int*) &nodeId) == -1) {
				putdebugError("handleMessage", "ID id da mensagem inválido");
				return -1;
			}

			//testar o valor do identificador pretendido
			if(nodeId > MAXID) {
				putdebugError("handleMessage", "o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
				return -1;
			}

			error = handleID(nodeId, fd);

		} else if(strcmp(command, "BOOT") == 0 && argCount == 1) {	//mensagem BOOT

			//verificar que foi o predi quem enviou BOOT
			if(fd != prediNode.fd) {
				putdebugError("handleMessage", "BOOT feito sem ser pelo predi");
				return -1;
			}

			putdebug("mensagem BOOT");
			iAmStartNode = TRUE;
			error = 0;

		} else if(strcmp(command, "END") == 0) {	//mensagem END
			putdebug("mensagem de END");

			int start = FALSE;
			if(argCount == 4) {
				putdebug("nó de arranque ainda não foi encontrado");
			} else if(strcmp(arg[3], "START") == 0 && argCount == 5) {
				putdebug("nó de arranque ainda já foi encontrado");
				start = TRUE;
			} else {
				putdebugError("handleMessage", "mensagem inválida");
				return -1;
			}

			int id;
			if(stringToUInt(arg[0], (unsigned int*) &id) == -1) {
				putdebugError("handleMessage", "END id da mensagem inválido");
				return -1;
			}

			//testar o valor do identificador pretendido
			if(id > MAXID) {
				putdebugError("handleMessage", "o identificador do nó está limitado ao intervalo [0-%d]\n", MAXID);
				return -1;
			}

			//arg[1] - endereco IP
			//arg[2] - porto

			if( (error = handleEND(id, arg[1], arg[2], start)) == -1) {
				putdebugError("handleMessage", "END falhou");
			}

		} else if(strcmp(command, "RING") == 0  && argCount == 3) {	//mensagem RING
			putdebug("mensagem de RING");

			int id, ring;
			if(stringToUInt(arg[0], (unsigned int*) &ring) == -1 ||
					stringToUInt(arg[1], (unsigned int*) &id)) {
				putdebugError("handleMessage", "RING id ou ring da mensagem inválido");
				return -1;
			}

			if( (error = handleRING(ring, id)) == -1) {
				putdebugError("handleMessage", "RING falhou");
			}

		} else {
			putdebugError("handleMessage", "mensagem inválida");
		}
	}

	return error;
}

int handleNEW(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	if(prediNode.fd == -1) {	//insercao do segundo nó no anel

		//definir novo predi
		prediNode.id = id;
		strcpy(prediNode.ip, ip);
		strcpy(prediNode.port, port);
		prediNode.fd = fd;

		if(succiNode.fd == -1) {

			if( (succiNode.fd = connectToNode(ip, port)) == -1) {
				putdebugError("handleNEW", "ligação ao novo succi /%s, %s) falhou", ip, port);
			} else {
				//adicionar ao conjunto
				addConnection(succiNode.fd);

				//definir novo succi
				succiNode.id = id;
				strcpy(succiNode.ip, ip);
				strcpy(succiNode.port, port);

				//enviar mensagem de CON ao succi
				if( (error = sendMessageCON(id, ip, port, succiNode.fd)) == -1) {
					putdebugError("handleNEW", "mensagem de CON não enviada ao succi");
				}
			}
		} else {
			if(insertingInRing != -1) {
				curRing = insertingInRing;
				insertingInRing = -1;

				putmessage("novo nó inserido no anel %d com succi %d %s %s\n",
								curRing, succiNode.id, succiNode.ip, succiNode.port);
			}

			error = 0;
		}

	} else {

		if(id != succiNode.id) {
			//enviar mensagem de CON ao succi
			if( (error = sendMessageCON(id, ip, port, prediNode.fd)) == -1) {
				putdebugError("handleNEW", "mensagem de CON não enviada ao succi");
			}
		} else {
			if(insertingInRing != -1) {
				curRing = insertingInRing;
				insertingInRing = -1;

				putmessage("novo nó inserido no anel %d com succi %d %s %s\n",
								curRing, succiNode.id, succiNode.ip, succiNode.port);
			}
		}

		putdebug("terminei ligacao %d", prediNode.fd);
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

int handleCON(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	if(curNode.id == id) {	//testar se o id do no recebido é o mesmo do no actual
		//existem apenas 2 nós no anel
		//definir predi = succi e predi.fd = %d nova ligacao
		prediNode.id = succiNode.id;
		strcpy(prediNode.ip, succiNode.ip);
		strcpy(prediNode.port, succiNode.port);
		prediNode.fd = fd;

		if(insertingInRing != -1) {
			curRing = insertingInRing;
			insertingInRing = -1;

			putmessage("novo nó inserido no anel %d com succi %d %s %s\n",
							curRing, succiNode.id, succiNode.ip, succiNode.port);
		}

		if(prediNode.fd == succiNode.fd) {
			//penultimo nó saiu do anel - fica apenas este nó no anel
			closeConnection(&prediNode.fd);
			closeConnection(&succiNode.fd);
			prediNode.id = succiNode.id = -1;
		}

		error = 0;

	} else {
		//terminar ligacao com succi
		putdebug("terminei ligacao com o succi actual %d", succiNode.fd);
		closeConnection(&succiNode.fd);

		//ligar ao no recebido
		if( (succiNode.fd = connectToNode(ip, port)) == -1) {
			putdebugError("handleCON", "ligação ao novo succi falhou");
		} else {
			//adicionar ao conjunto
			addConnection(succiNode.fd);

			//definir novo succi
			succiNode.id = id;
			strcpy(succiNode.ip, ip);
			strcpy(succiNode.port, port);

			putdebug("ligação estabelecida com o novo succi %s %s", succiNode.ip, succiNode.port);

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
int handleQRY(int searcherId, int searchedId, int *ownerId, char *ownerIp, char *ownerPort) {
	int error = -1;

	//testar se nó pertence a algum anel
	if(curRing == -1) {
		//nó não pertence a um anel logo não pode fazer uma pesquisa
		putdebugError("handleQRY", "nó não pertence a um anel logo não pode fazer uma pesquisa");
		return -1;
	}

	//passar query ao succi
	putdebug("enviar QRY a succi (%d, %s, %s, %d)", succiNode.id, succiNode.ip, succiNode.port, succiNode.fd);
	if( (error = sendMessageQRY(succiNode.fd, searcherId, searchedId)) == -1) {
		putdebugError("handleQRY", "envio de mensagem de QRY falhou");

	} else {

		char answer[BUFSIZE];
		bzero(answer, sizeof(answer));
		//esperar resposta
		if( (error = waitForRSP(succiNode.fd, answer, searcherId, searchedId, ownerId, ownerIp, ownerPort)) == -1) {
			putdebugError("handleQRY", "espera por resposta falhou");
		} else {

			if(searcherId == curNode.id) {
				//sou o nó que iniciou a procura
				error = 1;
			} else {
				//passar resposta para o predi
				if( (error = sendMessageRSP(prediNode.fd, searcherId, searchedId,
							*ownerId, ownerIp, ownerPort)) == -1) {
					putdebugError("handleQRY", "passagem da resposta para o predi falhou");
				}
			}
		}
	}

	return error;
}

#include <signal.h>
#define ID_INTERVAL	2					//tempo em segundos entre IDs
volatile int idAvailable = TRUE;		//define quando o ID está disponivel

/*
 * descricao: funcao que trata o sinal de alarme gerado pelo tratamento da mensagem ID
 * Nota: este alarme é utilizado para garantir que o nó está num estado estável entre dois IDs
 */
void timeExpired(int signal)
{
    if(signal == SIGALRM)
    {
    	putdebug("tempo de espera entre IDs espirou");
        idAvailable = TRUE;
    }
}

int handleID(int id, int fd) {
	int error = -1;

	//definir alarme para contar tempo entre IDs
	signal(SIGALRM, timeExpired);

	//testar se esta disponivel
	if(!idAvailable) {
		putdebugError("handleID", "insercoes estao ocupadas");
		//notificar fonte da pesquisa
		sendMessage(fd, "BUSY\n");
		return 0;
	}

	//definir que novas insercoes nao esta disponiveis temporariamente
	alarm(ID_INTERVAL);
	idAvailable = FALSE;

	Node succNode;

	if(distance(id, curNode.id) < distance(id, prediNode.id)) {
		//nó é responsavel pelo id procurado
		succNode.id = curNode.id;
		strcpy(succNode.ip, curNode.ip);
		strcpy(succNode.port, curNode.port);

	} else {

		if( (error = handleQRY(curNode.id, id, &succNode.id, succNode.ip, succNode.port)) != 1) {
			putdebugError("handleID", "pesquisa pelo succi falhou");
			return -1;
		}
	}

	//enviar succi encontrado
	if( (error = sendSUCC(fd, &succNode)) == -1) {
		putdebugError("handleID", "envio de SUCC ao nó falhou");
		error = -1;
	} else {

		//fechar ligacao
		putdebug("terminei ligacao com o nó %d", fd);
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

int rebuild() {
	int error = 0;

	if(succiNode.fd == -1 || FD_ISSET(prediNode.fd, &readFds)) { // ligacao com succi inexistente
		if(prediNode.fd == -1) {		// ligacao com predi inexistente
			//ficou ultimo nó no anel
			putdebug("saida abrupta: nó passa a ser o único nó no anel");

			if(!iAmStartNode) {
				//registar-se num novo anel
				if(registerNewRing() == -1) {
					putdebugError("handleEND", "não foi possível registar novo anel");
					return -1;
				}

				putdebug("registado num novo anel %d", curRing);
			}

			putmessage("anel reconstruído com sucesso após saída abrupta\n");

		} else {
			//existem mais nós no anel
			putdebug("saida abrupta");

			// enviar mensagem com os seus dados ao predi
			if(sendMessageEND(curNode.id, curNode.ip, curNode.port, prediNode.fd, iAmStartNode) == -1) {
				putdebugError("rebuild", "envio de mensagem END a predi falhou");
				return -1;
			}
		}
	}

	return error;
}

int handleEND(int id, const char *ip, const char *port, int start) {
	int error = 0;

	if(prediNode.fd == -1 || FD_ISSET(prediNode.fd, &readFds)) {  // testar se so o nó solto no anel
		//sou o nó solto no anel
		putdebug("sou a ponta solta no anel (start=%d)", start);

		if(!start && !iAmStartNode) {
			putdebug("anel perdeu o nó de arranque");

			//registar  num novo anel
			if(registerNewRing() == -1) {
				putdebugError("handleEND", "não foi possível registar novo anel");
				return -1;
			}

			putdebug("registado num novo anel %d", curRing);
		}

		//estabelecer ligacao com nó recebido
		int fd = -1;
		if( (fd = connectToNode(ip, port)) == -1) {
			putdebugError("handleEND", "não foi possivel ligar à outra ponta do anel");
			return -1;
		}

		//enviar uma mensagem de CON para restabelecer o anel
		if(sendMessageCON(curNode.id, curNode.ip, curNode.port, fd) == -1) {
			putdebugError("handleEND", "não foi possivel enviar CON à outra ponta do anel");
			return -1;
		}

		//terminar ligação estabelecida
		closeConnection(&fd);

		putmessage("anel reconstruído com sucesso após saída abrupta\n");

	} else {
		//não sou o nó solto no anel
		putdebug("não sou a ponta solta no anel");

		//se for o nó de arranque anunciar que foi encontrado
		if(iAmStartNode)
			start = TRUE;

		//retransmitir mensagem para o predi
		if(sendMessageEND(id, ip, port, prediNode.fd, start) == -1) {
			putdebugError("handleEND", "envio de mensagem END a predi falhou");
			return -1;
		}
	}

	return error;
}

int handleRING(int ring, int id) {
	int error = 0;

	//definir novo id do anel
	curRing = ring;

	putmessage("novo anel %d\n", curRing);
	putmessage("anel reconstruído com sucesso após saída abrupta\n");

	if(succiNode.id == id || succiNode.fd == -1) {
		putdebug("mensagem RING chegou à outra ponta do anel");
	} else {
		putdebug("retransmitir RING");

		//retransmitir mensagem
		if(sendMessageRING(succiNode.fd, curRing, id) == -1) {
			putdebugError("handleRING", "retransmissao de RING falhou");
			return -1;
		}
	}

	return error;
}

int registerNewRing() {
	int error = 0;

	int registerRing = -1;
	Node startNode;		// nó de arranque
	int n = curNode.id + 1;		//usar id para ser diferente
	while(n < (INT_MAX - 1)) {
		int testRing = curRing + n;
		putdebug("tentar registar como anel %d", testRing);

		int errorCode = getStartNode(testRing, &startNode);
		if(errorCode == -1) {
			putdebugError("rebuild", "obtencao de nó de arranque falhou");
			return -1;
		} else if(errorCode == 0) {
			//não existe nenhum anel com este id
			registerRing = testRing;
			break;

		} else {

			//estabelecer ligacao com nó de arranque registado
			int fd = -1;
			if( (fd = connectToNode(startNode.ip, startNode.port)) == -1) {
				//nó de arranque foi perdido
				putdebug("anel esta disponivel");
				registerRing = testRing;
				break;
			} else {
				putdebug("anel nao esta disponivel");
				//terminar ligação
				close(fd);

				//exprimentar proximo id para o anel
				n++;
			}
		}
	}

	if(registerRing >= 0) {
		//registar anel e como nó de arranque
		if(registerAsStartingNode(registerRing, &curNode) == -1) {
			putdebugError("handleEND", "registo no servidor falhou");
			return -1;
		} else {
			//registo com successo notificar outros nos no anel
			curRing = registerRing;
			iAmStartNode = TRUE;

			if(succiNode.fd != -1) {
				if(sendMessageRING(succiNode.fd, curRing, curNode.id) == -1) {
					putdebugError("handleEND", "envio de RING falhou");
					return -1;
				} else {
					putmessage("novo anel %d\n", curRing);
				}
			}
		}
	}


	if(n == INT_MAX) {
		putdebugError("handleEND", "não existe nenhum anel disponivel");
		//não foi possivel reconstruir o anel
		curRing = -1;
		return -1;
	}

	return error;
}
