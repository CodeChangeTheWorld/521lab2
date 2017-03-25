#include "trap_handlers.h"
#include <stdio.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "mem_management.h"
#include "context_switch.h"
#include "load_program.h"
#include "terminals.h"


void getpid_handler(ExceptionInfo *info);
void delay_handler(ExceptionInfo *info);
void exit_handler(ExceptionInfo *info);
void fork_trap_handler(ExceptionInfo *info);
void wait_trap_handler(ExceptionInfo *info);
void exec_trap_handler(ExceptionInfo *info);
void tty_read_handler(ExceptionInfo *info);
void tty_write_handler(ExceptionInfo *info);

#define SCHEDULE_DELAY 2
int time_till_switch= SCHEDULE_DELAY;

void kernel_trap_handler(ExceptionInfo *info){

    int code = info->code;
    switch(code){
        case YALNIX_FORK:
            fork_trap_handler(info);
            break;
        case YALNIX_EXEC:
            exec_trap_handler(info);
            break;
        case YALNIX_EXIT:
            exit_handler(info,0);
            break;
        case YALNIX_WAIT:
            wait_trap_handler(info);
            break;
        case YALNIX_GETPID:
            getpid_handler(info);
            break;
        case YALNIX_BRK:
            brk_handler(info);
            break;
        case YALNIX_DELAY:
            delay_handler(info);
            break;
        case YALNIX_TTY_READ:
            tty_read_handler(info);
            break;
        case YALNIX_TTY_WRITE:
            tty_write_handler(info);
            break;
    }
}
void fork_trap_handler(ExceptionInfo *info){

}
void exec_trap_handler(ExceptionInfo *info){

}
void exit_handler(ExceptionInfo *info,int error){

}
void wait_trap_handler(ExceptionInfo *info){

}
void getpid_handler(ExceptionInfo *info){

}

void delay_handler(ExceptionInfo *info){

}
void tty_read_handler(ExceptionInfo *info){

}


void tty_write_handler(ExceptionInfo *info){

}
void clock_trap_handler (ExceptionInfo *info){

}
void illegal_trap_handler (ExceptionInfo *info){

}
void memory_trap_handler (ExceptionInfo *info){

}
void math_trap_handler (ExceptionInfo *info){

}

void tty_recieve_trap_handler (ExceptionInfo *info){

}

void tty_transmit_trap_handler (ExceptionInfo *info){

}

void reset_time_till_switch() {

}