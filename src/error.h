#ifndef SRC_ERROR_H_
#define SRC_ERROR_H_

extern int debug;		//indica se se pretende imprimir mensagens de debug ou nao

void putdebugError(const char *functionName, const char *format, ...);
void putdebug(const char *format, ...);

void puterror(const char *format, ...);
void putmessage(const char *format, ...);

#endif /* SRC_ERROR_H_ */
