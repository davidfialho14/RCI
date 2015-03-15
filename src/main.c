#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "common.h"
#include "communication.h"

int main(int argc, char const *argv[]) {

	//inicializacao
	if(initializeCommunication(argc, argv) != 0) exit(-1); //inicializacao falhou
	putok("inicializacao completa\n");

	return 0;
}
