#include <string.h>
#include <math.h>

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

int stringToUInt(const char *string, int *number) {
	int error = -1;
	int length = strlen(string);
	*number = 0;

	for(int i = 0; i < length; ++i) {
		if(string[i] < '0' || string[i] > '9') {
			error = -1;
			break;
		} else {
			error = 0;
			*number = *number * 10 + (string[i] - '0');
		}
	}

	return error;
}
