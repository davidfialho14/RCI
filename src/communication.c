#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include "communication.h"
#include "string_operations.h"
#include "common.h"
#include "network_operations.h"

Node curNode;
int curRing = -1;	//definido a -1 quando o no nao esta registado em nenhuma anel
Node prediNode;
Node succiNode;
char startServerIp[IPSIZE];
char startServerPort[PORTSIZE];
int startServerFd = -1;
struct sockaddr startServerAddress;

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
		putwarning("leitura dos argumentos de entrada");
		return initialized;
	}
	putok("argumentos de entrada lidos com sucesso");	//debug

	if( (initialized = listenSocket()) != 0) {
		putwarning("listen socket falhou");
		return initialized;
	}
	putok("socket de escuta criado");	//debug

	if( (initialized = startServerSocket()) != 0) {
		putwarning("socket do servidor de arranque falhou");
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
		putwarning("usage: ddt [-t ringport] [-i bootip] [-p bootport]");
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
		putok("ringport: %s", curNode.port);	//debug
		putok("bootIP: %s", startServerIp);		//debug
		putok("bootport: %s", startServerPort);	//debug
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
	hints.ai_family = AF_INET; 			//permitir o uso de IPv4 e IPv6
	hints.ai_socktype = SOCK_STREAM;	//definir como TCP
	hints.ai_flags = AI_PASSIVE;		//usar endereco IP da maquina

	if(getHostnameAddress(curNode.ip) == 0) {

		struct addrinfo *servinfo;		//lista de enderecos
		//obter enderecos do servidor de arranque
		if (getaddrinfo(NULL, curNode.port, &hints, &servinfo) != 0) {
			puterror("listenSocket", "getaddrinfo falhou");

		} else {
			//iterar pelos varios enderecos ate conseguir criar um socket e fazer bind
			struct addrinfo *aux = servinfo;
			while(aux != NULL) {
				if( (curNode.fd = socket(aux->ai_family,
						aux->ai_socktype, aux->ai_protocol)) != -1) {
					//criado um socket com sucesso
					//indicar ao socket para dispensar o porto quando a aplicacao termina
					int opt = -1;
					setsockopt(curNode.fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
							sizeof(int));

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
		puterror("startServerSocket", "getaddrinfo falhou");

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
			puterror("startServerSocket",
					"nao foi criado nenhum socket para o servidor de arranque");
		}

		//limpar recursos
		freeaddrinfo(servinfo);
	}

	return error;
}
