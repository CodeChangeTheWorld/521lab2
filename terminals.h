#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct charbuffer {
    char buffer[TERMINAL_MAX_LINE];
    int read;
    int write;
    int count;
};

struct charbuffer *charbuffers;

void init_charbuffers();
