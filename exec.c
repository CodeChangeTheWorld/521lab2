//
// Created by Liu Fang on 3/31/17.
//
#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>


int main(){
    if (i == TTY_CONSOLE)
        cmd_argv[0] = "console";
    else
        cmd_argv[0] = "shell";
    sprintf(numbuf, "%d", i);
    cmd_argv[1] = numbuf;
    cmd_argv[2] = NULL;
    Exec(cmd_argv[0], cmd_argv);
}