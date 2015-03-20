#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "common.h"

extern int errno;
FILE *logFile = NULL;

void openLogFile() {
	if( (logFile = fopen(".ddt.log", "a+")) == NULL) {
		puts("ERRO: ficheiro de log não pode ser aberto");
	}
}

void closeLogFile() {
	if(logFile != NULL)
		fclose(logFile);
	logFile = NULL;
}

void putwarning(const char *format, ...) {
	openLogFile();
	if(logFile != NULL) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);
		fprintf(logFile, "WARN: ");
		vfprintf(logFile, format, argp);
		fprintf(logFile, "\n");
		va_end(argp);
		closeLogFile();
	}
}

void puterror(const char *functionName, const char *format, ...) {
	openLogFile();
	if(logFile != NULL) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);
		fprintf(logFile, "ERRO: %s ", functionName);
		vfprintf(logFile, format, argp);
		if(errno != 0) {
			fprintf(logFile, ": %s", strerror(errno));
		}
		fprintf(logFile, "\n");
		va_end(argp);
		closeLogFile();
	}
}

void putok(const char *format, ...) {
	openLogFile();
	if(logFile != NULL) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);

		fprintf(logFile, "OK: ");
		vfprintf(logFile, format, argp);
		fprintf(logFile, "\n");

		va_end(argp);
		closeLogFile();
	}
}

void putdebug(const char *format, ...) {
	openLogFile();
	if(logFile != NULL) {
		va_list argp;			//lista de argumentos
		va_start(argp, format);

		fprintf(logFile, "DEBUG: ");
		vfprintf(logFile, format, argp);
		fprintf(logFile, "\n");

		va_end(argp);
		closeLogFile();
	}
}

void putmessage(const char *format, ...) {
	va_list argp;			//lista de argumentos
	va_start(argp, format);

	fprintf(stdout, "> ");
	vfprintf(stdout, format, argp);

	va_end(argp);
}
