#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int main(){
    printf("Test tty_write Process Initialized.\n");
    char *buf = malloc(sizeof(char) * 4);
    buf[0] = 'h';
    buf[1] = 'e';
    buf[2] = 'l';
    buf[3] = 'l';
    buf[4] = 'o';

    TtyWrite(0,buf,5);
    printf("Wrote to terminal 0 \n");
    return 0;
}