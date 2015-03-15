#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "ring_interface.h"
#include "defines.h"
#include "common.h"
#include "communication.h"

int handleMessage(const char* message, int fd) {
	int error = -1;

	char command[BUFSIZE];	//comando da mensagem
	char arg[5][IPSIZE];	//argumentos da mensagem
	char extra[BUFSIZE];	//utilizado nao para testar se existe um argumento a mais

	//filtrar mensagem
	int argCount = sscanf(message, "%s %s %s %s %s %s %s", command, arg[0], arg[1], arg[2],
															arg[3], arg[4], extra);

	if(strcmp(command, "QRY") == 0 && argCount == 3) {	//QRY message
		putok("mensagem QRY");

		int searcherId = atoi(arg[0]);
		int searchedId = atoi(arg[1]);

		if(searchedId == 0 || searcherId == 0) {
			puterror("handleMessage", "QRY ids invalidos");
			return -1;
		}

		error = 0;

	} else if(strcmp(command, "CON") == 0 && argCount == 4) {
		putok("mensagem de CON");

		//int id = atoi(arg[0]);
		//arg[1] - endereco IP
		//arg[2] - porto

		error = 0;

	} else if(strcmp(command, "RSP") == 0 && argCount == 6) {
		putok("mensagem RSP");
		error = 0;
	} else if(strcmp(command, "NEW") == 0 && argCount == 4) {
		putok("mensagem de NEW");

		//int id = atoi(arg[0]);
		//arg[1] - endereco IP
		//arg[2] - porto

		error = 0;

	} else if(strcmp(command, "ID") == 0 && argCount == 2) {
		putok("mensagem ID");
		error = 0;
	} else if(strcmp(command, "SUCC") == 0 && argCount == 4) {
		putok("mensagem SUCC");
		error = 0;
	} else if(strcmp(command, "BOOT") == 0 && argCount == 1) {
		putok("mensagem BOOT");
		error = 0;
	} else {
		puterror("handleMessage", "mensagem invalida");
	}

	return error;
}



