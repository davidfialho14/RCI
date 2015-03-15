#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include "network_operations.h"
#include "defines.h"

int getHostnameAddress(char address[HOST_NAME_MAX]) {
	int error = -1;

	struct ifaddrs *ifaddrs;			//lista de interfaces
	if(getifaddrs(&ifaddrs) != -1) {	//obter lista de interfaces
		char host[HOST_NAME_MAX];			//endereco

		struct ifaddrs *ifa = ifaddrs;	//primeira interface
		while(ifa != NULL) {			//iterar pela interfaces ate encontrar uma que nao seja o localhost
			sa_family_t family = ifa->ifa_addr->sa_family;
			if(family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
				if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
						HOST_NAME_MAX, NULL, 0, NI_NUMERICHOST) == 0) {
					//retornar endereco da interface
					strcpy(address, host);
					error = 0;
				}
			}
			ifa = ifa->ifa_next;
		}
		//limpar recursos ocupados pela lista
		freeifaddrs(ifaddrs);
	}

	return error;
}
