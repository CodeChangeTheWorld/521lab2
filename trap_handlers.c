//
// Created by Liu Fang on 3/19/17.
//

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);
void exit_handler(ExceptionStackFrame *frame);
void fork_trap_handler(ExceptionStackFrame *frame);
void wait_trap_handler(ExceptionStackFrame *frame);
void exec_trap_handler(ExceptionStackFrame *frame);
void tty_read_handler(ExceptionStackFrame *frame);
void tty_write_handler(ExceptionStackFrame *frame);

#define SCHEDULE_DELAY 2
int time_till_switch= SCHEDULE_DELAY;

void kernel_trap_handler(ExceptionStackFrame *frame){

    int code = frame->code;
    switch(code){
        case YALNIX_FORK:
            fork_trap_handler(frame);
            break;
        case YALNIX_EXEC:
            exec_trap_handler(frame);
            break;
        case YALNIX_EXIT:
            exit_handler(frame,0);
            break;
        case YALNIX_WAIT:
            wait_trap_handler(frame);
            break;
        case YALNIX_GETPID:
            getpid_handler(frame);
            break;
        case YALNIX_BRK:
            brk_handler(frame);
            break;
        case YALNIX_DELAY:
            delay_handler(frame);
            break;
        case YALNIX_TTY_READ:
            tty_read_handler(frame);
            break;
        case YALNIX_TTY_WRITE:
            tty_write_handler(frame);
            break;
    }
}
void fork_trap_handler(ExceptionStackFrame *frame){

}
void exec_trap_handler(ExceptionStackFrame *frame){

}
void exit_handler(ExceptionStackFrame *frame,int error){
    int exit_status;
    if(error){
        exit_status = ERROR;
    } else{
        exit_status = frame->regs[1];
    }

    struct schedule_item *current =get_head();
    if(!is_current_process_orphan()){
        struct process_control_bloc *parent_pcb = get_pcb_by_pid(current->pcb->parent_pid);
        parent_pcb->is_waiting = 0;
        add_child_exit_status(parent_pcb,exit_status,get_current_pid());
    }

    decapitate();
}
void wait_trap_handler(ExceptionStackFrame *frame){
    int *status = (int *)frame->regs[1];
    struct schedule_item *head = get_head();
    struct process_control_block *parent_pcb = head->pcb;

    struct exit_status_node *esn = pop_next_child_exit_status_node(parent_pcb);
    if(esn == NULL){
        if(parent_pcb->num_children == 0){
            frame->reg[0] = (long)NULL;
            return;
        }
        parent_pcb->is_waiting = 1;
        reset_time_till_switch();
        schedule_processes();
        esn = pop_next_child_exit_status_node(parent_pcb);
    }
    *status_ptr = esn->exit_status;
    frame->regs[0] = esn->pid;
}
void getpid_handler(ExceptionStackFrame *frame){
    frame->regs[0] = get_current_pid();
}

void delay_handler(ExceptionStackFrame *frame){
    int num_ticks_to_wait = frame->regs[1];
    if(num_ticks_to_wait < 0){
        frame->regs[0] = ERROR;
        return;
    }
    struct schedule_item_ *item = get_head();
    struct process_control_block *current_pcb = item->pcb;
    current_pcb->delay = num_ticks_to_wait;

    frame->regs[0]=0;
    if(num_ticks_to_wait >0){
        schedule_processes();
    }
    return;;
}
void tty_read_handler(ExceptionStackFrame *frame){

}
void tty_write_handler(ExceptionStackFrame *frame){

}
void clock_trap_handler (ExceptionStackFrame *frame){

}
void illegal_trap_handler (ExceptionStackFrame *frame){

}
void memory_trap_handler (ExceptionStackFrame *frame){

}
void math_trap_handler (ExceptionStackFrame *frame){

}
void tty_recieve_trap_handler (ExceptionStackFrame *frame){

}

void tty_transmit_trap_handler (ExceptionStackFrame *frame){

}
void
reset_time_till_switch() {

}