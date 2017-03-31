#include "trap_handlers.h"
#include <stdio.h>
#include "process_scheduling.h"
#include "pcb.h"
#include "mem_management.h"
#include "context_switch.h"
#include "load_program.h"
#include "terminals.h"


void getpid_handler(ExceptionInfo *info);
void delay_handler(ExceptionInfo *info);
void exit_handler(ExceptionInfo *info, int error);
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
            TracePrintf(1,"trap_handlers: Fork requested.");
            fork_trap_handler(info);
            break;
        case YALNIX_EXEC:
            TracePrintf(1,"trap_handlers: Exec requested.");
            exec_trap_handler(info);
            break;
        case YALNIX_EXIT:
            TracePrintf(1,"trap_handlers: Exit requested.");
            exit_handler(info,0);
            break;
        case YALNIX_WAIT:
            TracePrintf(1,"trap_handlers: Wait requested.");
            wait_trap_handler(info);
            break;
        case YALNIX_GETPID:
            TracePrintf(1,"trap_handlers: GetPID requested.");
            getpid_handler(info);
            break;
        case YALNIX_BRK:
            TracePrintf(1,"trap_handlers: Brk requested.");
            brk_handler(info);
            break;
        case YALNIX_DELAY:
            TracePrintf(1,"trap_handlers: Delay requested.");
            delay_handler(info);
            break;
        case YALNIX_TTY_READ:
            TracePrintf(1,"trap_handlers: TTYRead requested.");
            tty_read_handler(info);
            break;
        case YALNIX_TTY_WRITE:
            TracePrintf(1,"trap_handlers: TTYWrite requested.");
            tty_write_handler(info);
            break;
    }
}


void fork_trap_handler(ExceptionInfo *info){
    TracePrintf(1, "trap_handlers: fork attempted, but there is not enough memory for REGION_1 copy.\n");
    struct schedule_item *item = get_head();
    ProcessControlBlock *parent_pcb = item->pcb;

    int child_pid = get_next_pid();
    ProcessControlBlock *child_pcb = create_new_process(child_pid , get_current_pid());

    TracePrintf(3, "%p\n", child_pcb->page_table);
    ContextSwitch(init_region_0_for_child, &parent_pcb->saved_context,(void*)parent_pcb,(void*) child_pcb);

    if(parent_pcb->out_of_memory){
        TracePrintf(1, "trap_handlers: fork attempted, but there is not enough memory for REGION_1 copy.\n");
        struct schedule_item *current = get_head();
        remove_head();
        info->regs[0] = ERROR;
    }else{
        if(get_current_pid()==child_pid){
            info->regs[0] = 0;
        }else{
            info->regs[0] = child_pid;
            parent_pcb->num_children++;
        }
    }
}

void exec_trap_handler(ExceptionInfo *info){
    char *filename = (char *)info->regs[1];
    char *argvec = (char *)info->regs[2];

    struct schedule_item *item = get_head();
    int load_return_val = LoadProgram(filename,argvec,info,item->pcb->page_table);
    if(load_return_val==-1){
        info->regs[0] = ERROR;
    }
}

void exit_handler(ExceptionInfo *info,int error){
    int exit_status;
    if(error){
        exit_status = ERROR;
    }else{
        exit_status = info->regs[1];
    }
    struct  schedule_item *current = get_head();
    TracePrintf(3,"trap_handlers: parent: %d\n", current->pcb->pid);
    if(!is_current_process_orphan()){
        ProcessControlBlock * parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
        parent_pcb->is_waiting = 0;
        add_child_exit_status();
    }
    decapitate();
}
void wait_trap_handler(ExceptionInfo *info){

}
void getpid_handler(ExceptionInfo *info){
    info->regs[0] = get_current_pid();
}

void delay_handler(ExceptionInfo *info){
    int num_ticks_to_wait = info->regs[1];
    if(num_ticks_to_wait < 0){
        info->regs[0] = ERROR;
        return;
    }
    struct schedule_item *item =get_head();
    ProcessControlBlock *current_pcb = item->pcb;
    current_pcb->delay = num_ticks_to_wait;

    info->regs[0] =0;
    if(num_ticks_to_wait >0){
        schedule_processes();
    }
    return;
}

void tty_read_handler(ExceptionInfo *info){
    int terminal = info->regs[1];
    if(terminal < 0 || terminal > NUM_TERMINALS){
        info->regs[0] = ERROR;
        return;
    }
    void* buf = (void*)info->regs[2];
    int len = info->regs[3];
    int num_read = read_from_buffer(terminal,buf,len);
    if(num_read >= 0){
        info->regs[0] = num_read;
    }else{
        info->regs[0] = ERROR;
    }
}


void tty_write_handler(ExceptionInfo *info){
    int terminal = info->regs[1];
    if(terminal < 0 || terminal > NUM_TERMINALS){
        info->regs[0] = ERROR;
        return;
    }
    void* buf = (void*)info->regs[2];
    int len = info->regs[3];

    struct schedule_item *item = get_head();
    ProcessControlBlock *current_pcb = item->pcb;

    int num_written = write_to_buffer(terminal, buf,len);
    TtyTransmit(terminal, buf, num_written);

    current_pcb->is_writing_to_terminal = terminal;

    if(num_written>0){
        info->regs[0] = num_written;
    }else{
        info->regs[0] = ERROR;
    }
}

void clock_trap_handler (ExceptionInfo *info){
    time_till_switch--;
    decrement_delays();
    TracePrintf(3, "trap_handler: time_till_switch: %d\n", time_till_switch);
    if(time_till_switch == 0) {
        reset_time_till_switch();
        schedule_processes();
    }
}

void illegal_trap_handler (ExceptionInfo *info){
    int code = info->code;
    int current_pid = get_current_pid();
    printf("trap_handlers: Terminating current process of pid %d due to TRAP_ILLEGAL of code %d\n", current_pid,code);
    exit_handler(info,1);
}
void memory_trap_handler (ExceptionInfo *info){
    struct schedule_item *item = get_head();
    ProcessControlBlock *pcb = item->pcb;

    int code = info->code;
    void *addr = info->addr;
    void *user_stack_limit = pcb->user_stack_limit;
    void *brk = pcb->brk;

    TracePrintf(2,"trap_handler: TRAP_MEMORY with addr %p\n",addr);

    if(DOWN_TO_PAGE(addr) < DOWN_TO_PAGE(user_stack_limit)){
      if(UP_TO_PAGE(addr) > UP_TO_PAGE(brk)+ PAGESIZE){
          grow_user_stack(addr,pcb); //in mem_management
          return;
      }
    }

    switch (code) {
        case TRAP_MEMORY_MAPERR:
            printf("TRAP_MEMORY was due to: No mapping at a virtual address\n");
            break;
        case SEGV_ACCERR:
            printf("TRAP_MEMORY was due to: Protection violation at a virtual address\n");
            break;
        case SI_KERNEL:
            printf("TRAP_MEMORY was due to: Linux kernel sent SIGSEGV at virtual address\n");
            break;
        case SI_USER:
            printf("TRAP_MEMORY was due to: Received SIGSEGV from user\n");
            break;
    }

    exit_handler(info,1);
}
void math_trap_handler (ExceptionInfo *info){
    TracePrintf(1,"trap_handlers: Entering TRAP_MATH interrupt handler\n");
    int code = info->code;
    int current_pid = get_current_pid();
    printf("trap_handlers: Terminating current process of pid %d due to TRAP_MATH of code %d",current_pid,code);
    exit_handler(info, 1);
}

void tty_recieve_trap_handler (ExceptionInfo *info){
    int terminal = info->code;
    char *receivechars = malloc(sizeof(char) * TERMINAL_MAX_LINE);
    int num_chars_received = TtyReceive(terminal, receivechars, TERMINAL_MAX_LINE);
    write_to_buffer_raw(terminal,receivechars,num_chars_received);

    if(new_line_in_buffer(terminal)){
        wake_up_a_reader_for_terminal(terminal);
    }
    TracePrintf(1, "trap_handlers: Received %d chars from terminal %d.\n", num_chars_received, terminal);
}

void tty_transmit_trap_handler (ExceptionInfo *info){
    int terminal = info->code;
    ProcessControlBlock *pcb= get_pcb_writing_to_terminal(terminal);

    if (pcb == NULL) {
        TracePrintf(3, "trap_handlers: on tty_transmit, could not find the process writing to terminal %d.\n", terminal);
    } else {
        TracePrintf(3, "trap_handlers: process with pid %d found to have been writing to terminal %d.\n", pcb->pid, terminal);
    }

    wake_up_a_writer_for_terminal(terminal);
    TracePrintf(1,"trap_handlers: TRAP_TTY_TRANSMIT handler finished.\\n");
}

void reset_time_till_switch() {
    time_till_switch = SCHEDULE_DELAY;
}
