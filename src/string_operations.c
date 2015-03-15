#include <string.h>

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
