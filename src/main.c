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
#include "communication.h"
#include "connections_set.h"
#include "error.h"
#include "ring_interface.h"
#include "user_interface.h"

int main(int argc, char const *argv[]) {

	//inicializacao
	if(initializeCommunication(argc, argv) != 0) exit(-1); //inicailizacao falhou
	putdebug("inicialização completa\n");

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
		putmessage("\r> ");
		inputReady = select(maxFd + 1, &readFds, NULL, NULL, NULL);
		if(inputReady <= 0) {
			putdebugError("main", "select falhou");
			continue;
		}

		if(FD_ISSET(curNode.fd, &readFds)) {	//testar se o listen fd esta pronto para leitura
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);
			bzero(&addr, addrlen);

			//aceitar ligacao
			int connectionFd;
			if( (connectionFd = accept(curNode.fd, (struct sockaddr*)&addr, &addrlen)) == -1) {
				putdebugError("main", "ligação não foi aceite");
			} else {
				putdebug("nova ligação %d com endereço: %s", connectionFd, inet_ntoa(addr.sin_addr));
				//adicionar descritor ao conjunto de descritores de ligacao
				addConnection(connectionFd);

				//definir um timeout para as comunicações
				struct timeval tv;
				tv.tv_sec = 5;
				setsockopt(connectionFd, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
				setsockopt(connectionFd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval));
			}
		}

		if(FD_ISSET(STDIN_FILENO, &readFds)) { //testar se o utilizador executou um comando

			//ler comando do utilizador
			bzero(buffer, sizeof(buffer));
			if(fgets(buffer, BUFSIZE, stdin) != NULL) {
				//executar comando do utilizador
				putdebug("processar comando do utilizador");

				int errorCode = executeUserCommand(buffer);
				switch(errorCode) {
					case 0: 	putdebug("comando de utilizador processado com sucesso"); break;
					case -1: 	putdebugError("main", "falha no processamento do comando de utilizador"); break;
					case 1:
					{
						putmessage("programa vai sair\n");

						//fechar todos os sockets
						int fd = getFirstConnection();
						while(fd >= 0) {
							close(fd);
							rmConnection(fd);
							//proxima ligacao
							fd = getNextConnection(fd);
						}

						//fechar socket de escuta e socket do servidor de arranque
						closeSockets();

						quit = TRUE;
						continue;
					}
				}
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
					putdebug("ligacao %d terminada", connectionFd);

					if(connectionFd == prediNode.fd) {
						prediNode.fd = -1;
						prediNode.id = -1;
						putdebug("ligação terminada com predi");
					}

					if(connectionFd == succiNode.fd) {
						succiNode.fd = -1;
						succiNode.id = -1;
						putdebug("ligação terminada com succi");

						// possivel tentativa de reconstrucao do anel
						if(rebuild() == -1) {
							puterror("não foi possível reconstruir o anel\n");
							putmessage("vai ser feito um reset ao nó\n");

							//fechar todas as ligações com predi e succi
							closeConnection(&succiNode.fd);
							closeConnection(&prediNode.fd);

							curNode.id = succiNode.id = prediNode.id = -1;

							if(iAmStartNode) {
								//remover anel do servidor
								unregisterRing(curRing);
								curRing = -1;
								iAmStartNode = FALSE;
							}

						} else {
							putmessage("foi feita uma reconstrução do anel com sucesso\n");
						}
					}

				} else {
					//tratar mensagem recebida
					putdebug("mensagem recebida pela ligacao %d: %s", connectionFd, buffer);

					//remover \n
					int length = strlen(buffer);
					if(buffer[length - 1] == '\n')
						buffer[length - 1] = '\0';

					if(handleMessage(buffer, connectionFd) == -1) {
						//notificar quem enviou o pedido que ocorreu um erro
						char answer[] = "ERROR\n";
						sendMessage(connectionFd, answer);

						if(connectionFd == prediNode.fd) {
							prediNode.fd = -1;
							prediNode.id = -1;
							putdebug("ligação terminada com predi por causa de erro");
						}

						if(connectionFd == succiNode.fd) {
							succiNode.fd = -1;
							succiNode.id = -1;
							putdebug("ligação terminada com succi por causa de erro");
						}

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
