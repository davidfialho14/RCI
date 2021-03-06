#include <string.h>
#include <limits.h>

#include "string_operations.h"
#include "defines.h"

int isStringNumber(const char *string) {
	int isNumber = FALSE;
	int length = strlen(string);

	for(int i = 0; i < length; ++i) {
		if(string[i] < '0' || string[i] > '9') {
			isNumber = FALSE;
			break;
		} else {
			isNumber = TRUE;
		}
	}

	return isNumber;
}

int stringToUInt(const char *string, unsigned int *number) {
	int error = -1;
	int length = strlen(string);
	long n = 0;

	for(int i = 0; i < length; ++i) {
		if(string[i] < '0' || string[i] > '9') {
			error = -1;
			break;
		} else {
			error = 0;
			n = n * 10 + (string[i] - '0');
		}
	}

	if(n > UINT_MAX) {
		//number is bigger than a unsigned int
		error = -1;
	} else {
		*number = (unsigned int) n;
	}

	return error;
}
