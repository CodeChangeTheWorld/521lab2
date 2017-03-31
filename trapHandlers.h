//
// Created by Liu Fang on 3/19/17.
//

#include <comp421/hardware.h>
#include <comp421/yalnix.h>

void kernel_trap_handler(ExceptionInfo *info);

void clock_trap_handler (ExceptionInfo *info);

void illegal_trap_handler (ExceptionInfo *info);

void memory_trap_handler (ExceptionInfo *info);

void math_trap_handler (ExceptionInfo *info);

void tty_recieve_trap_handler (ExceptionInfo *info);

void tty_transmit_trap_handler (ExceptionInfo *info);

void reset_time_till_switch();
