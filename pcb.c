#include "process_control_block.h"
#include "process_scheduling.h"
#include "page_table_management.h"
#include "stddef.h"
#include "stdlib.h"


void add_child_exit_status(struct process_control_block *parent_pcb, int exit_status, int child_pid) {
    struct exit_status_node *current = parent_pcb->exit_status_queue;
    struct exit_status_node *new_exit_status_node = malloc(sizeof(struct exit_status_node));
    new_exit_status_node->exit_status = exit_status;
    new_exit_status_node->pid = child_pid;
    new_exit_status_node->next = NULL;
    if(current == NULL){
        parent_pcb->exit_status_queue = new_exit_status_node;
    }else{
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_exit_status_node;
    }
}

/*
 *  Function to create an idle process.
 *  Return ProcessControlBlock* Pointer to the pcb of idle process.
 */
ProcessControlBlock* create_idle_process(){
    ProcessControlBlock *pcb = create_empty_process(IDLE_PID, ORPHAN_PARENT_PID);
    init_initial_page_table(pcb->page_table);
    return pcb;
}

/*
 *  Function to create an new process.
 *  Return ProcessControlBlock* Pointer to the pcb of the new process.
 */
ProcessControlBlock* create_new_process(int pid, int parent_id){
    ProcessControlBlock *pcb = create_empty_process(pid,parent_id);
    init_page_table(pcb->page_table);
    return pcb;
}

/*
 *  Function to create an empty process.
 *  Return ProcessControlBlock* Pointer to the pcb of the empty process.
 */
ProcessControlBlock* create_empty_process(int pid, int parent_pid){
    ProcessControlBlock *pcb = malloc(sizeof(struct process_control_block));
    pcb->pid=pid;
    pcb->page_table= create_page_table();
    pcb->delay =0;
    pcb->exit_status_queue=NULL;
    pcb->parent_pid=parent_pid;
    pcb->out_of_memory = 0;
    pcb->is_waiting = 0;
    pcb->num_children = 0;
    pcb->is_waiting_to_read_from_terminal = -1;
    pcb->is_writing_to_terminal =-1;
    pcb->is_waiting_to_write_to_terminal = -1;
    add_to_schedule(pcb);
    return pcb;
}
