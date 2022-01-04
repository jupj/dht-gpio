#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "convert.h"

int convert(char *port) {
    if (strlen(port) <3) {
        return CONVERT_INVALID_PORT;
    }

    if (port[0] != 'P') {
        return CONVERT_INVALID_PORT;
    }

    if (!isupper(port[1])) {
        return CONVERT_INVALID_PORT;
    }

    for (int i=2; i<strlen(port); i++) {
        if (!isdigit(port[i])) {
            return CONVERT_INVALID_PORT;
        }
    }

    int bank = port[1] - 'A';
    int num = atoi(&port[2]);

    return 32*bank + num;
}
