#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "common.h"

extern int errno;

void putwarning(const char *format, ...) {

	va_list argp;			//lista de argumentos
	va_start(argp, format);
	fprintf(stdout, "WARN: ");
	vfprintf(stdout, format, argp);
	fprintf(stdout, "\n");
	va_end(argp);
}

void puterror(const char *functionName, const char *format, ...) {

	va_list argp;			//lista de argumentos
	va_start(argp, format);
	fprintf(stderr, "ERRO: %s ", functionName);
	vfprintf(stderr, format, argp);
	if(errno != 0) {
		fprintf(stderr, ": %s", strerror(errno));
	}
	fprintf(stderr, "\n");
	va_end(argp);
}

void putok(const char *format, ...) {
	va_list argp;			//lista de argumentos
	va_start(argp, format);
	fprintf(stdout, "OK: ");
	vfprintf(stdout, format, argp);
	fprintf(stdout, "\n");
	va_end(argp);
}

void putdebug(const char *format, ...) {
	va_list argp;			//lista de argumentos
	va_start(argp, format);
	fprintf(stdout, "DEBUG: ");
	vfprintf(stdout, format, argp);
	fprintf(stdout, "\n");
	va_end(argp);
}
