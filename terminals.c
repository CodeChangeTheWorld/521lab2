//
// Created by Liu Fang on 3/19/17.
//
#include "terminals.h"
#include "process_scheduling.h"
#include "process_control_block.h"
#include "trap_handlers.h"


int new_line_in_buffer(int terminal);

void init_charbuffers(){
    charbuffers = malloc(sizeof(struct charbuffer)* NUM_TERMINALS);
    int i;
    for(i=0;i<NUM_TERMINALS; i++){
        charbuffers[i].read =0;
        charbuffers[i].write =0;
    }
}

int new_line_in_buffer(int terminal){
    int i;
    int buffer_index = charbuffers[terminal].read;
    for(i=0;i<charbuffers[terminal].count;i++){
        TracePrintf();
        if(charbuffers[terminal].buffer[buffer_index] == '\n'){
            return 1;
        }
        buffer_index = (buffer_index+1) % TERMINAL_MAX_LINE;
    }
    return 0;
}

int read_from_buffer(int terminal, char *buf, int len){
    int i;
    struct schedule_item *item = get_head();
    struct process_control_block *current_pcb = item->pcb;

    while(!new_line_in_buffer(terminal)){
        TracePrintf(3,"terminals: waiting to read aline from terminal %d\n",terminal);
        current_pcb->is_waiting_to_write_to_terminal = terminal;
        reset_time_till_switch();
        schedule_processes();
    }

    int num_read = 0;
    int buffer_index = charbuffers[terminal].read;
    for(i=0; i<len;i++){
        if(charbuffers[terminal].count > 0){
            buf[i] = charbuffers[terminal].buffer[buffer_index];
            buffer_index = (buffer_index + 1)%TERMINAL_MAX_LINE;
            charbuffers[terminal].count--;
            num_read++;
        }else{
            break;
        }
    }
    TracePrintf(2,"terminals: read %d characters\n", num_read);
    return num_read;
}

int write_to_buffer_raw(int terminal, char *buf, int len){
    int i;
    int num_written=0;
    for(int i=0;i<len ;i++){
        if(charbuffers[terminal].count != TERMINAL_MAX_LINE){ // drop characters if buffer is full
            charbuffers[terminal].buffer[charbuffers[terminal].write] = buf[i];
            charbuffers[terminal].write = (charbuffers[terminal].write+1) % TERMINAL_MAX_LINE;
            charbuffers[terminal].count++;
            num_written++;
        }
    }
    return num_written;
}
int write_to_buffer(int terminal, char *buf, int len){
    int i;
    while(get_pcb_by_pid()){

    }
}