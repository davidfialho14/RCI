#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "defines.h"
#include "error.h"

extern int errno;
int debug = FALSE;

void putdebugError(const char *functionName, const char *format, ...) {
	if(debug) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);

		fprintf(stdout, "ERRO: %s: ", functionName);
		vfprintf(stdout, format, argp);
		if(errno != 0) {
			fprintf(stdout, ": %s", strerror(errno));
		}
		fprintf(stdout, "\n");
		fflush(stdout);

		va_end(argp);
	}
}

void putdebug(const char *format, ...) {
	if(debug) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);

		fprintf(stdout, "DEBUG: ");
		vfprintf(stdout, format, argp);
		if(errno != 0) {
			fprintf(stdout, ": %s", strerror(errno));
		}
		fprintf(stdout, "\n");

		va_end(argp);
		closeLogFile();
	}
}


void puterror(const char *format, ...) {

	va_list argp;			//lista de argumentos
	va_start(argp, format);

	fprintf(stderr, "erro: ");
	vfprintf(stderr, format, argp);
	if(errno != 0) {
		fprintf(stderr, ": %s", strerror(errno));
	}
	fprintf(stderr, "\n");
	fflush(stderr);

	va_end(argp);
}

void putmessage(const char *format, ...) {
	va_list argp;			//lista de argumentos
	va_start(argp, format);

	vfprintf(stdout, format, argp);
	fflush(stdout);

	va_end(argp);
}
