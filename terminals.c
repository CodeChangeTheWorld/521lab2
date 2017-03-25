//
// Created by Liu Fang on 3/19/17.
//
#include "terminals.h"
#include "process_scheduling.h"
#include "process_control_block.h"
#include "trap_handlers.h"

void init_charbuffers(){
    charbuffers = malloc(sizeof(struct charbuffer)* NUM_TERMINALS);
    int i;
    for(i=0;i<NUM_TERMINALS; i++){
        charbuffers[i].read =0;
        charbuffers[i].write =0;
    }
}