#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "processQueue.h"
#include "contextSwitch.h"
#include "trapHandlers.h"
#include "pageTableManagement.h"
#include "pcb.h"

int can_idle_switch();
int next_pid = BASE_PID;
ProcessControlBlock *head = NULL;

void decrement_delays(){
    ProcessControlBlock *current = head;
    while(current != NULL){
        if(current->delay > 0){
            current->delay--;
        }
        current = current->next;
    }
}

int get_current_pid(){
    return get_head()-> pid;
}

int get_next_pid(){
    return next_pid++;
}

int can_idle_switch(){
    ProcessControlBlock *current = head->next;
    while(current!=NULL){
        if(current->delay==0){
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void move_head_to_tail(){
    if(head->next == NULL) return;
    if(head != NULL){
        ProcessControlBlock *current = get_head();
        ProcessControlBlock *new_head = current->next;
        while(current->next != NULL){
            current = current->next;
        }
        head->next = NULL;
        current->next = head;
        head = new_head;
    }
}
void add_to_schedule(ProcessControlBlock* pcb){
    pcb -> next = head;
    head = pcb;
}

ProcessControlBlock *get_head() {
    return head;
}

void remove_head(){
    ProcessControlBlock *current = get_head();
    head = current->next;
    free_page_table(current->page_table);
    free(current);
}

int is_current_process_orphan(){
    ProcessControlBlock *head = get_head();
    return head->parent_pid == ORPHAN_PARENT_PID;
}

ProcessControlBlock* get_pcb_by_pid(int pid){
    ProcessControlBlock *current = get_head();
    while(current!=NULL){
        if(current->pid == pid){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void decapitate(){
    ProcessControlBlock *current = get_head();
    if(current == NULL){
        TracePrintf(0, "process_scheduling: decapitate when there is no process. ");
        Halt();
    }
    if(current->pid == IDLE_PID){
        TracePrintf(0, "process_scheduling: decapitate IDLE process. ");
        Halt();
    }
    schedule_process_during_decap();
    remove_head();
    TracePrintf(2, "process_scheduling: decapitate finished. ");
}

void schedule_process_during_decap(){

    ProcessControlBlock *old_head = get_head();

    //exclude old_head of process selection
    head = old_head->next;
    select_next_process();
    ProcessControlBlock *next_head = get_head();

    ContextSwitch(MyContextSwitch, &old_head->saved_context,(void*)old_head, (void*)next_head);
    reset_time_till_switch();
}

void schedule_processes(){
    TracePrintf(2, "process_scheduling: Begin schedule_processes");
    ProcessControlBlock *current_pcb = get_head();

    if(current_pcb->pid != 1 || can_idle_switch()){
        move_head_to_tail();
        select_next_process();

        ProcessControlBlock *next_pcb = get_head();
        ContextSwitch(MyContextSwitch, &current_pcb->saved_context,(void*)current_pcb, (void*)next_pcb);

        reset_time_till_switch();
    }
}

void select_next_process(){
    TracePrintf(2, "process_scheduling: Beginning select next process. ");
    if(move_next_process_to_head(0)){
        return;
    }else if(move_next_process_to_head(IDLE_DELAY)){
        return;
    }
    Halt();
}

void move_next_process_to_head(int delay){
    TracePrintf(2,"process_scheduling: begin move_next_process_to_head.");
    ProcessControlBlock *current_pcb = head;
    ProcessControlBlock *previous_pcb = NULL;

    while(current!= NULL){
        if(current_pcb->delay == delay && current_pcb->is_waiting == 0 
            && current_pcb->is_waiting_to_read_from_terminal == -1 && current_pcb->is_waiting_to_write_to_terminal== -1 
                && current_pcb->is_writing_to_terminal == -1){
            if(previous_pcb == NULL){
                return 1;
            }else{
                previous_pcb->next = current_pcb->next;
                current_pcb->next = head;
                head = current_pcb;
                return 1;
            }
        }else{
            previous_pcb = current_pcb;
            current_pcb = current_pcb->next;
        }
    }
    return 0;
}

ProcessControlBlock * get_pcb_writing_to_terminal(int terminal){
    TracePrintf(3, "process_scheduling: looking for process writing to terminals %d\n", terminal);
    ProcessControlBlock *current= get_head();

    while(current!=NULL){
        TracePrintf(3, "process_scheduling: current has pid of %d\n", current->pid);
        TracePrintf(3, "process_scheduling: current is writing to terminal: %d\n", current->is_writing_to_terminal);
        if(current->is_writing_to_terminal == terminal){
            return current;
        }
        current = current->next;
    }

    TracePrintf(3, "process_scheduling: did not find process writing to terminal %d\n", terminal);
    return NULL;
}

void wake_up_a_writer_for_terminal(int terminal){
    ProcessControlBlock *current = get_head();
    while(current != NULL){
        if(current->is_waiting_to_write_to_terminal == terminal){
            current->is_waiting_to_write_to_terminal = -1;
            return;
        }
        current = current->next;
    }
    return;
}

void wake_up_a_reader_for_terminal(int terminal){
    ProcessControlBlock *current = get_head();

    while(current != NULL){
        if(current->is_waiting_to_read_from_terminal == terminal){
            current->is_waiting_to_read_from_terminal = -1;
            return;
        }
        current = current->next;
    }
    return;
}
