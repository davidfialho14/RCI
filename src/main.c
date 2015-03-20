#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "defines.h"
#include "common.h"
#include "communication.h"
#include "connections_set.h"
#include "ring_interface.h"
#include "user_interface.h"

int main(int argc, char const *argv[]) {

	//inicializacao
	if(initializeCommunication(argc, argv) != 0) exit(-1); //inicailizacao falhou
	putok("inicializacao completa\n");

	//inicializar conjunto de ligacoes
	initializeConnectionSet();

	int maxFd = curNode.fd;	//descritor com valor mais elevado
	char buffer[BUFSIZE];	//buffer utilizado para fazer a leitura dos descritores
	int inputReady = 0;		//indica se existe input disponivel para ler
	fd_set readFds;			//conjunto de fds prontos para ler
	int quit = FALSE;
	while(!quit) {

		//reinicializar conjunto de fds de leitura
		FD_ZERO(&readFds);
		//adicionar ligacoes no conjunto de fds
		copySet(&readFds);
		//adicionar listen fd ao conjunto de fds
		FD_SET(curNode.fd, &readFds);
		//adicionar stdin ao conjunto de fds
		FD_SET(STDIN_FILENO, &readFds);

		putdebug("CurNode - ring: %d id: %d ip: %s port: %s fd: %d",
				curRing, curNode.id, curNode.ip, curNode.port, curNode.fd);
		putdebug("SucciNode - id: %d ip: %s port: %s fd: %d",
						succiNode.id, succiNode.ip, succiNode.port, succiNode.fd);
		putdebug("PrediNode - id: %d ip: %s port: %s fd: %d",
						prediNode.id, prediNode.ip, prediNode.port, prediNode.fd);

		if(maxFd < getMaxConnection()) {
			maxFd = getMaxConnection();
		}

		//esperar por descritor pronto para ler
		printf("> ");
		inputReady = select(maxFd + 1, &readFds, NULL, NULL, NULL);
		if(inputReady <= 0) {
			puterror("main", "select falhou");
			continue;
		}

		if(FD_ISSET(curNode.fd, &readFds)) {	//testar se o listen fd esta pronto para leitura
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);
			bzero(&addr, addrlen);

			//aceitar ligacao
			int connectionFd;
			if( (connectionFd = accept(curNode.fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
				puterror("main", "ligação não foi aceite");
			} else {
				putdebug("nova ligação %d addr: %s %d", connectionFd, inet_ntoa(addr.sin_addr));
				//adicionar descritor ao conjunto de descritores de ligacao
				addConnection(connectionFd);
			}
		}

		if(FD_ISSET(STDIN_FILENO, &readFds)) { //testar se o utilizador executou um comando

			//ler comando do utilizador
			fgets(buffer, BUFSIZE, stdin);

			//executar comando do utilizador
			putdebug("processar comando do utilizador");

			int errorCode = executeUserCommand(buffer);
			switch(errorCode) {
				case 0: 	putok("comando de utilizador processado com sucesso"); break;
				case -1: 	puterror("main", "falha no processamento do comando"); break;
				case 1:		putmessage("programa vai sair"); quit = TRUE; continue;
			}
		}

		//ler fds de ligacoes actuais com o nó
		int connectionFd = getFirstConnection();
		while(connectionFd >= 0) {
			if(FD_ISSET(connectionFd, &readFds)) {
				//limpar buffer de rececao
				bzero(buffer, sizeof(buffer));

				//ler mensagem
				if(readMessage(connectionFd, buffer, sizeof(buffer)) <= 0) {
					close(connectionFd);		//fechar ligacao
					rmConnection(connectionFd);	//remover no do conjunto de ligacoes
					putok("ligacao %d terminada", connectionFd);

					if(connectionFd == succiNode.fd) {
						succiNode.fd = -1;
					}

					if(connectionFd == prediNode.fd) {
						prediNode.fd = -1;
					}

				} else {
					//tratar mensagem recebida
					putok("mensagem recebida pela ligacao %d: %s", connectionFd, buffer);

					//remover \n
					int length = strlen(buffer);
					if(buffer[length - 1] == '\n')
						buffer[length - 1] = '\0';

					if(handleMessage(buffer, connectionFd) == -1) {
						//notificar quem enviou o pedido que ocorreu um erro
						char answer[] = "ERROR\n";
						sendMessage(connectionFd, answer);
						//terminar ligacao
						close(connectionFd);
						rmConnection(connectionFd);
					}
				}
			}

			//proxima ligacao
			connectionFd = getNextConnection(connectionFd);
		}
	}

	return 0;
}
