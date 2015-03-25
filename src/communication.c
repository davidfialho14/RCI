#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "communication.h"

#include "error.h"
#include "string_operations.h"
#include "network_operations.h"

Node curNode;
int curRing = -1;	//definido a -1 quando o no nao esta registado em nenhuma anel
Node prediNode;
Node succiNode;
char startServerIp[IPSIZE];
char startServerPort[PORTSIZE];
int startServerFd = -1;
struct sockaddr startServerAddress;
int iAmStartNode = FALSE;

/*****************
 * Inicialização *
 *****************/

int readInputArgs(int argc, const char *argv[]);
int listenSocket();
int startServerSocket();
int initializeCommunication(int argc, const char *argv[]) {
	int initialized = 0;

	//iniciar com valores por defeito
	curNode.id = curNode.fd = -1;
	prediNode.id = prediNode.fd = -1;
	succiNode.id = succiNode.fd = -1;

	if( (initialized = readInputArgs(argc, argv)) != 0) {
		putdebug("initializeCommunication", "leitura dos argumentos de entrada");
		return initialized;
	}
	putok("argumentos de entrada lidos com sucesso");	//debug

	if( (initialized = listenSocket()) != 0) {
		puterror("listen socket falhou");
		return initialized;
	}
	putok("socket de escuta criado");	//debug

	if( (initialized = startServerSocket()) != 0) {
		puterror("socket do servidor de arranque falhou");
		return initialized;
	}
	putok("socket de so servidor de arranque criado");	//debug

	return initialized;
}

/*
 *	descricao:	le argumentos de entrada e testa se o seu formato esta correcto.
 * 				caso o formato esteja correcto os valores do porto TCP, do endereco
 * 				do servidor de arranque e do porto do servidor de arranque sao armazenados
 * 				nas variaveis globais correspondentes
 * 	retorno:	retorna 0 em caso de sucesso e um valor menor que zero caso contrario
 * 				com o codigo do tipo de erro
 * 	erros:		EARGS - formato incorrecto dos argumentos de entrada
 */
int readInputArgs(int argc, const char *argv[]) {
	int error = -1;		//definido como nao lidos correctamente
	int inputMode = -1;		//indica o modo de entrada:
							//	1-sem endereco do servidor de arranque
							//	2-com endereco do servidor de arranque
	if(argc == 3) {
		inputMode = 1;		//sem endereco do servidor de arranque
	} else if(argc == 7) {
		inputMode = 2;		//com endereco do servidor de arranque
	} else {
		puterror("usage: ddt [-t ringport] [-i bootip] [-p bootport]");
		return -1;
	}

	int ringportDefined = FALSE, bootIPDefined = FALSE, bootportDefined = FALSE;

	//ler argumentos 1 a 1
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-t") == 0) {
			//mover para o proximo argumento
			i++;
			if(i < argc && !ringportDefined && isStringNumber(argv[i])) {
				//guardar porto TCP na variavel global
				strcpy(curNode.port, argv[i]);
				ringportDefined = TRUE;
			}
		} else if(strcmp(argv[i], "-i") == 0) {
			//mover para o proximo argumento
			i++;
			if(i < argc && !bootIPDefined) {
				//guardar endereco ip do servidor de arranque na variavel global
				strcpy(startServerIp, argv[i]);
				bootIPDefined = TRUE;
			}
		} else if(strcmp(argv[i], "-p") == 0) {
			i++;
			if(i < argc && !bootportDefined && isStringNumber(argv[i])) {
				//guardar porto do servidor de arranque na variavel global
				strcpy(startServerPort, argv[i]);
				bootportDefined = TRUE;
			}
		}
	}

	//verificar se foram lidos argumentos de acordo com modo de entrada
	if(inputMode == 1 && ringportDefined) {
		//endereco e porto do servidor de arranque por defeito
		strcpy(startServerIp, "tejo.tecnico.ulisboa.pt");
		strcpy(startServerPort, "58000");
		error = 0;
	} else if(inputMode == 2 && ringportDefined && bootIPDefined && bootportDefined) {
		error = 0;
	}

	if(error == 0) {
		putmessage("Dados de arranque\n");
		putmessage("ringport: %s\n", curNode.port);	//debug
		putmessage("bootIP: %s\n", startServerIp);		//debug
		putmessage("bootport: %s\n", startServerPort);	//debug
	}

	return error;
}

/*
 * 	descricao:	cria o socket de tipo TCP/IP utilizado para escutar por novas ligacoes
 * 				feitas pelos varios nos
 * 	retorno:	0 em caso de sucesso e -1 em caso de erro
 */
int listenSocket() {
	int error = -1;

	struct addrinfo hints;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET; 				//permitir o uso de IPv4 e IPv6
	hints.ai_socktype = SOCK_STREAM;	//definir como TCP
	hints.ai_flags = AI_PASSIVE;			//usar endereco IP da maquina

	if(getHostnameAddress(curNode.ip) == 0) {
		putmessage("host ip: %s\n", curNode.ip);
		struct addrinfo *servinfo;		//lista de enderecos
		//obter enderecos do host
		if (getaddrinfo(NULL, curNode.port, &hints, &servinfo) != 0) {
			putdebug("listenSocket", "não foi possível obter o endereço para fzer bind");
		} else {
			//iterar pelos varios enderecos ate conseguir criar um socket e fazer bind
			struct addrinfo *aux = servinfo;
			while(aux != NULL) {
				if( (curNode.fd = socket(aux->ai_family, aux->ai_socktype, aux->ai_protocol)) != -1) {
					//criado um socket com sucesso
					//indicar ao socket para dispensar o porto quando a aplicacao termina
					int opt = -1;
					setsockopt(curNode.fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(int));

					//fazer bind ao endereco obtido
					if(bind(curNode.fd, aux->ai_addr,  aux->ai_addrlen) == -1) {
						close(curNode.fd);	//fechar socket criado
						curNode.fd = -1;
					} else {
						//colocar socket a escuta por ligacoes
						if(listen(curNode.fd, 5) == -1) {
							close(curNode.fd);
							curNode.fd = -1;
						} else {
							error = 0;	//indica que o socket foi criado com sucesso
							break;
						}
					}
				}

				aux = aux->ai_next;
			}
			//limpar recursos
			freeaddrinfo(servinfo);
		}
	}

	return error;
}

/*
 *	descricao:	cria um novo socket do tipo UDP/IP para comunicar com o servidor de arranque
 */
int startServerSocket() {
	int error = -1;

	struct addrinfo hints;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET; 		//permitir o uso de IPv4 e IPv6
	hints.ai_socktype = SOCK_DGRAM;	//definir como TCP

	struct addrinfo *servinfo;
	if (getaddrinfo(startServerIp, startServerPort, &hints, &servinfo) != 0) {
		putdebug("startServerSocket", "não foi possível obter endereço do servidor de arranque");

	} else {
		//iterar pelos varios enderecos ate conseguir criar um socket com sucesso
		struct addrinfo *aux = servinfo;
		while(aux != NULL) {
			if( (startServerFd = socket(aux->ai_family, aux->ai_socktype, aux->ai_protocol)) != -1) {
				//criado um socket com sucesso
				//copiar sockaddr utilizado pelo socket para usar durante as comunicacoes
				memcpy(&startServerAddress, aux->ai_addr, aux->ai_addrlen);
				error = 0;
				break;
			}

			aux = aux->ai_next;
		}

		if(aux == NULL) {
			putdebug("startServerSocket", "não foi criado nenhum socket para o servidor de arranque");
		}

		//limpar recursos
		freeaddrinfo(servinfo);
	}

	return error;
}

/*
 * TERMINAÇÃO
 */
void closeSockets() {
	if(curNode.fd != -1)
		close(curNode.fd);
	if(startServerFd != -1)
		close(startServerFd);
}

/****************************************
 * Comunicacao com servidor de arranque *
 ****************************************/

int getStartNode(int ringId, Node* startNode) {
	int error = -1;

	//criar mensagem de pedido
	char message[BUFSIZE];
	sprintf(message, "BQRY %d", ringId);

	//enviar pedido
	if(sendto(startServerFd, message, strlen(message), 0,
			&startServerAddress, sizeof(startServerAddress)) <= 0) {
		putdebug("getStartNode", "envio de pedido de registo falhou");
	} else {
		//limpar buffer
		bzero(message, sizeof(message));
		socklen_t addrlen = sizeof(startServerAddress);	//comprimento do endereco

		//receber reposta do servidor de arranque
		if(recvfrom(startServerFd, message, sizeof(message), 0,
					&startServerAddress, &addrlen) <= 0) {
			putdebug("getStartNode", "resposta do servidor de arranque não recebida");

		} else {
			if(strcmp(message, "EMPTY") == 0) {
				//anel esta vazio
				error = 0;
			} else {

				char command[BUFSIZE];						//comando (esperado BRSP)
				int ring = -1, startNodeId = -1;	//numero do anel
				char startNodeIp[IPSIZE], startNodePort[IPSIZE];
				char extra[BUFSIZE];	//usado para testar se a resposta tem argumentos a mais

				//filtrar resposta
				int argCount = sscanf(message, "%s %d %d %s %s %s", command, &ring, &startNodeId,
											 	startNodeIp, startNodePort, extra);

				//testar se o numero do anel na resposta corresponde ao anel do pedido
				if(ring != ringId) {
					putdebug("getStartNode",
						"resposta servidor de arranque incorrecta: o anel introduzido e o anel recebido são diferentes");
				} else {

					//verificar resposta
					if(argCount == 5 && strcmp(command, "BRSP") == 0) {

						//retornar (por referencia) dados do nó de arranque do anel
						startNode->id = startNodeId;
						strcpy(startNode->ip, startNodeIp);
						strcpy(startNode->port, startNodePort);

						error = 1;	//o anel nao esta vazio e resposta tem foramto correcto
					} else {
						putdebug("getStartNode",
							"resposta do servidor de arranque incorrecta: comando não é BRSP");
					}
				}
			}
		}
	}

	return error;
}

int registerAsStartingNode(int ringId, const Node *node) {
	int error = -1;

	//criar mensagem para registar nó como nó de arranque
	char message[BUFSIZE];
	sprintf(message, "REG %d %d %s %s", ringId, node->id, node->ip, node->port);
	socklen_t addrlen = sizeof(startServerAddress);

	//enviar pedido
	if(sendto(startServerFd, message, strlen(message), 0, &startServerAddress, addrlen) > 0) {
		//limpar buffer de mensagem
		bzero(message, sizeof(message));
		//receber reposta do servidor de arranque
		if(recvfrom(startServerFd, message, sizeof(message), 0, &startServerAddress, &addrlen) <= 0) {
			putdebug("registerAsStartingNode",
				"resposta do servidor de arranque não foi recebida");
		} else {
			//verificar se a reposta do servidor esta correcta
			if(strcmp(message, "OK") == 0) {
				//mensagem de resposta correcta
				curNode.id = node->id;
				curRing = ringId;
				error = 0;
			} else {
				putdebug("registerAsStartingNode",
						"resposta do servidor de arranque incorrecta: resposta recebida não foi OK");
			}
		}
	}
	return error;
}

int unregisterRing(int ringId) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "UNR %d", ringId);

	//enviar pedido
	if(sendto(startServerFd, message, strlen(message), 0, &startServerAddress, sizeof(startServerAddress)) <= 0) {
		putdebug("getStartNode", "envio de pedido de registo falhou");
	} else {
		//limpar buffer
		bzero(message, sizeof(message));
		socklen_t addrlen = sizeof(startServerAddress);	//comprimento do endereco

		//receber reposta do servidor de arranque
		if(recvfrom(startServerFd, message, sizeof(message), 0, &startServerAddress, &addrlen) <= 0) {
			putdebug("getStartNode", "resposta do servidor de arranque não foi recebida");
		} else {
			if(strcmp(message, "OK") == 0) {
				error = 0;
			}
		}
	}

	return error;
}

/**************************
 * Comunicacao com os nós *
 **************************/

int connectToNode(const char *nodeAddress, const char *nodePort) {
	int nodeFd = -1;

	struct addrinfo hints;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET; 			//permitir o uso de IPv4
	hints.ai_socktype = SOCK_STREAM;	//definir como TCP

	struct addrinfo *servinfo;			//lista de enderecos
	//obter enderecos do succi
	if (getaddrinfo(nodeAddress, nodePort, &hints, &servinfo) != 0) {
		putdebug("connectToNode", "endereço do nó é inválido");
	} else {
		//iterar pelos varios enderecos ate conseguir criar um socket e fazer connect
		struct addrinfo *aux = servinfo;
		while(aux != NULL) {
			if( (nodeFd = socket(aux->ai_family,
					aux->ai_socktype, aux->ai_protocol)) != -1) {
				//criado um socket com sucesso
				//ligar ao endereco
				if(connect(nodeFd, aux->ai_addr,  aux->ai_addrlen) == -1) {
					putdebug("connectToNode", "tentativa de ligação a um nó falhada");
					close(nodeFd);	//fechar socket criado
					nodeFd = -1;
				} else {
					putok("ligação %d estabelecida com: %s %s", nodeFd, nodeAddress, nodePort);
					break;	//sair apos ligacao ter sido estabelecida
				}
			}
			aux = aux->ai_next;
		}

		if(aux == NULL) {
			putdebug("connectToNode", "não foi possível ligar ao nó %s %s", nodeAddress, nodePort);
		}

		//limpar recursos
		freeaddrinfo(servinfo);
	}

	return nodeFd;
}

int readMessage(int fd, char *message, size_t messageSize) {
	int bytesRead = 0;
	int totalBytesRead = 0;
	char buffer[BUFSIZE/4];
	char *lineFeed = NULL;

	int messageEnd = FALSE;
	while(!messageEnd) {

		bzero(buffer, sizeof(buffer));
		if( (bytesRead = read(fd, buffer, sizeof(buffer))) <= 0) {
			//ligacao terminada
			return -1;
		}

		if(totalBytesRead == 0) {
			//first bytes read
			strcpy(message, buffer);
		} else {
			if((totalBytesRead + bytesRead + 1) < messageSize) {
				//concatenar bytes lidos com os anteriores
				strcat(message, buffer);
			} else {
				//mensagem total demasiado grande
				putdebug("readMessage", "mensagem recebida excedeu limite de comprimento");
				return -1;
			}
		}

		if( (lineFeed = strchr(message, '\n')) != NULL) {
			messageEnd = TRUE;
			//remover \n
			*lineFeed = '\0';
		}

		totalBytesRead += bytesRead;
	}

	return totalBytesRead;
}

int sendMessage(int fd, const char *message) {
	int error = 0;
	int bytesLeft = strlen(message);
	int bytesWritten = 0;
	const char *ptr = message;

	while(bytesLeft > 0) {
		if( (bytesWritten = write(fd, ptr, bytesLeft)) <= 0) {
			putdebug("sendMessage", "envio de mensagem");
			error = -1;
			break;
		}
		bytesLeft -= bytesWritten;
		ptr += bytesWritten;
	}

	if(error == 0) {
		putok("mensagem enviada para fd %d: %s", fd, message);
	}

	return error;
}

int sendMessageNEW(int fd) {
	int error = -1;

	//criar mensagem new
	char message[BUFSIZE];
	sprintf(message, "NEW %d %s %s\n", curNode.id, curNode.ip, curNode.port);

	//enviar mensagem
	error = sendMessage(fd, message);

	return error;
}

int sendMessageCON(int id, const char *ip, const char *port, int fd) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "CON %d %s %s\n", id, ip, port);

	//enviar mensagem ao predi
	error = sendMessage(fd, message);

	return error;
}

int sendMessageQRY(int fd, int searcherId, int searchedId) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "QRY %d %d\n", searcherId, searchedId);

	//enviar mensagem ao succi
	error = sendMessage(fd, message);

	return error;
}

int waitForRSP(int fd, char *answer, int searcherId, int searchedId,
		int *ownerId, char *ownerIp, char *ownerPort) {
	int error = -1;

	//ler mensagem
	if(readMessage(fd, answer, sizeof(answer)) <= 0) {
		putdebug("waitForRSP", "ligação %d terminada durante espera por RSP", fd);
	} else {
		//mensagem recebida com successo

		char command[BUFSIZE];	//comando da resposta
		int l = 0, k = 0;
		char extra[BUFSIZE];

		//ler resposta
		if(sscanf(answer, "%s %d %d %d %s %s %s", command, &l, &k, ownerId, ownerIp, ownerPort, extra) == 6) {

			//verificar se os parametros da resposta batem certo com o pedido
			if(l != searcherId || k != searchedId) {
				putdebug("executeQRY", "resposta com parâmetros incorrectos: %s", answer);
			} else {
				//retornar id, ip e porto do nó responsavel
				error = 0;
			}

		} else {
			putdebug("executeQRY", "resposta recebida incorrecta: %s", answer);
		}
	}

	return error;
}

int sendMessageRSP(int fd, int searcherId, int searchedId, int ownerId,
		const char *ownerIp, const char *ownerPort) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "RSP %d %d %d %s %s\n", searcherId, searchedId, ownerId, ownerIp, ownerPort);

	//enviar mensagem ao predi
	error = sendMessage(fd, message);

	return error;
}

int sendMessageBOOT(int fd) {

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "BOOT\n");

	return sendMessage(fd, message);
}

int sendMessageID(int fd, int nodeId) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "ID %d\n", nodeId);

	//enviar mensagem ao predi
	error = sendMessage(fd, message);

	return error;
}

int waitForSUCC(int fd, Node *succNode) {
	int error = -1;

	char message[BUFSIZE];
	bzero(message, sizeof(message));	//limpar buffer
	if( (error = readMessage(fd, message, sizeof(message))) == -1) {
		putdebug("waitForSUCC", "leitura da resposta SUCC falhada");
	} else {

		char command[BUFSIZE];	//comando da resposta
		char extra[BUFSIZE];

		//filtrar mensagem
		if(sscanf(message, "%s %d %s %s %s", command,
				&succNode->id, succNode->ip, succNode->port, extra) == 4) {

			//resposta correcta
			error = 0;

		} else {
			putdebug("waitForSUCC", "mensagem de SUCC recebida incorrecta");
			error = -1;
		}
	}

	return error;
}

int sendSUCC(int fd, const Node *succNode) {
	int error = -1;

	//criar mensagem
	char message[BUFSIZE];
	sprintf(message, "SUCC %d %s %s\n", succNode->id, succNode->ip, succNode->port);

	//enviar mensagem ao predi
	error = sendMessage(fd, message);

	return error;
}
