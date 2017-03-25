//
// Created by Liu Fang on 3/19/17.
//

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
    int exit_status;
    if(error){
        exit_status = ERROR;
    } else{
        exit_status = info->regs[1];
    }

    struct schedule_item *current =get_head();
    if(!is_current_process_orphan()){
        struct process_control_bloc *parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
        parent_pcb->is_waiting = 0;
        add_child_exit_status(parent_pcb,exit_status,get_current_pid());
    }

    decapitate();
}
void wait_trap_handler(ExceptionInfo *info){
    int *status = (int *)info->regs[1];
    struct schedule_item *head = get_head();
    struct process_control_block *parent_pcb = head->pcb;

    struct exit_status_node *esn = pop_next_child_exit_status_node(parent_pcb);
    if(esn == NULL){
        if(parent_pcb->num_children == 0){
            info->reg[0] = (long)NULL;
            return;
        }
        parent_pcb->is_waiting = 1;
        reset_time_till_switch();
        schedule_processes();
        esn = pop_next_child_exit_status_node(parent_pcb);
    }
    *status_ptr = esn->exit_status;
    info->regs[0] = esn->pid;
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
    struct schedule_item_ *item = get_head();
    struct process_control_block *current_pcb = item->pcb;
    current_pcb->delay = num_ticks_to_wait;

    info->regs[0]=0;
    if(num_ticks_to_wait >0){
        schedule_processes();
    }
    return;
}
void tty_read_handler(ExceptionInfo *info){
    int terminal = info->regs[1];
    if(terminal < 0 || terminal>NUM_TERMINALS){
        info->regs[0] = ERROR;
        return;
    }
    void *buf = (void*)info->regs[2];
    int len = info->regs[3];

    int num_read = read_from_buffer(terminal, buf, len);
    if(num_read >= 0){
        info->regs[0] = num_read;
    }else{
        info->regs[0]= ERROR;
    }
}


void tty_write_handler(ExceptionInfo *info){
    int terminal = info->regs[1];
    if(terminal < 0 || terminal > NUM_TERMINALS){
        info->regs[0] = ERROR;
        return;
    }
    void *buf = (void*)info->regs[2];
    int len = info->regs[3];
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;

    int num_written = write_to_buffer(terminal,buf,len);
    TtyTransmit(terminal, buf, num_written);

    current_pcb->is_writing_to_terminal = terminal;

    if(num_written >= 0){
        info->regs[0] = num_written;
    }else{
        info->regs[0] = ERROR;
    }
}
void clock_trap_handler (ExceptionInfo *info){
    time_till_switch--;
    decrement_delays();
    if(time_till_switch == 0){
        reset_time_till_switch();
        schedule_processes();
    }
}
void illegal_trap_handler (ExceptionInfo *info){
    int code = info->code;
    int current_pid = get_current_pid();
    exit_handler(info,1);
}
void memory_trap_handler (ExceptionInfo *info){
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;

    int code = info->code;
    void *addr = info->addr;
    void *user_stack_limit = pcb->user_stack_limit;
    void *brk = pcb->brk;

    if(DOWN_TO_PAGE(addr) < DOWN_TO_PAGE(user_stack_limit)){
        if(UP_TO_PAGE(addr) > (UP_TO_PAGE(brk)+PAGESIZE)){
            grow_user_stack(addr, pcb);
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

    exit_handler(info, 1);
}
void math_trap_handler (ExceptionInfo *info){
    int code = info->code;
    int current_pid = get_current_pid();
    exit_handler(info,1);
}

void tty_recieve_trap_handler (ExceptionInfo *info){
    int terminal = info->code;
    char *receivedchars = malloc(sizeof(char) * TERMINAL_MAX_LINE);

    int num_received_chars = TtyReceive(terminal, receivedchars,TERMINAL_MAX_LINE);
    write_to_buffer_raw(terminal,receivedchars,num_received_chars);

    if(new_line_in_buffer(terminal)){
        wake_up_a_reader_for_terminal(terminal);
    }
}

void tty_transmit_trap_handler (ExceptionInfo *info){
    int terminal = info->code;
    struct process_control_block *done_writing_pcb = get_pcb_of_process_writing_to_terminal(terminal);

    if(done_writing_pcb != NULL){
       done_writing_pcb->is_writing_to_terminal = -1;
    }
    wake_up_a_writer_for_terminal(terminal);
}

void
reset_time_till_switch() {
    time_till_switch = SCHEDULE_DELAY;
}