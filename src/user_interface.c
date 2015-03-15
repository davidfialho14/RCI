#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "user_interface.h"
#include "defines.h"
#include "communication.h"
#include "common.h"

extern int curRing;

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

	} else if(strcmp(command, "exit") == 0 && argCount == 1) {	//comando exit?
		//o comando exit nao tem argumentos
		putok("comando exit");
		error = 1;	//indica que se pretende sair do programa

	} else {
		error = -1;
	}

	return error;
}
