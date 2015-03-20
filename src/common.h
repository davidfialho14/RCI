#ifndef SRC_COMMON_H_
#define SRC_COMMON_H_

void puterror(const char *format, ...);
void putwarning(const char *format, ...);
void putok(const char *format, ...);
void putdebug(const char *functionName, const char *format, ...);
void putmessage(const char *format, ...);

#endif /* SRC_COMMON_H_ */
