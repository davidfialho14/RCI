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

		fprintf(stdout, "\rERRO: %s: ", functionName);
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

		fprintf(stdout, "\rDEBUG: ");
		vfprintf(stdout, format, argp);
		fprintf(stdout, "\n");
		fflush(stdout);

		va_end(argp);
	}
}

void puterror(const char *format, ...) {

	va_list argp;			//lista de argumentos
	va_start(argp, format);

	fprintf(stderr, "\rerro: ");
	vfprintf(stderr, format, argp);
	fflush(stderr);

	va_end(argp);
}

void putmessage(const char *format, ...) {
	va_list argp;			//lista de argumentos
	va_start(argp, format);

	putchar('\r');
	vfprintf(stdout, format, argp);
	fflush(stdout);

	va_end(argp);
}
