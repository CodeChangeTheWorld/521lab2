#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "context_switch.h"
#include "trap_handlers.h"
#include "page_table_management.h"


int can_idle_switch();
int next_pid = BASE_PID;
struct schedule_item *head = NULL;

int can_idle_switch(){
    struct schedule_item *current = head->next;
    while(current!=NULL){
        struct process_control_block *pcb = current->pcb;
        if(pcb->delay==0){
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void move_head_to_tail(){
    if(head->next == NULL) return;
    if(head!=NULL){
        struct schedule_item *current = get_head();
        struct schedule_item *new_head = current->next;
        while(current->next!=NULL){
            current= current->next;
        }
        head->next = NULL;
        current->next = head;
        head= new_head;
    }
}
void add_to_schedule(struct process_control_block* pcb){
    struct schedule_item *item = malloc(sizeof(struct schedule_item));
    item->pcb = pcb;
    item->next = head;
    head = item;
}

struct schedule_item *get_head() {
    return head;
}

void remove_head(){
    struct schedule_item *current = get_head();
    head = current->next;
    free_page_table(current->pcb->page_table);
    free(current->pcb);
    free(current);
}

int is_current_process_orphan(){
    struct schedule_item *head = get_head();
    return head->pcb->parent_pid = ORPHAN_PARENT_PID;
}

struct process_control_block* get_pcb_by_pid(int pid){
    struct schedule_item *current = get_head();
    while(current!=NULL){
        if(current->pcb->pid == pid){
            return current->pcb;
        }
        current = current->next;
    }
    return NULL;
}

void decapitate(){
    struct schedule_item *current = get_head();
    if(current == NULL){
        TracePrintf(0, "process_scheduling: decapitate when there is no process. ");
        Halt();
    }
    struct process_control_block *current_pcb = current->pcb;
    if(current_pcb->pid == IDLE_PID){
        TracePrintf(0, "process_scheduling: decapitate IDLE process. ");
        Halt();
    }
    schedule_process_during_decap();
    remove_head();
    TracePrintf(2, "process_scheduling: decapitate finished. ");
}

void schedule_process_during_decap(){

    struct schedule_item *old_head = get_head();
    struct process_control_block *old_head_pcb = old_head->pcb;

    //exclude old_head of process selection
    head = old_head->next;
    select_next_process();
    struct schedule_item *next_head = get_head();
    struct process_control_block *next_pcb = next_head->pcb;

    ContextSwitch(MyContextSwitch, &old_head_pcb->saved_context,(void*)old_head_pcb, (void*)next_pcb);
    reset_time_till_switch();
}

void schedule_processes(){
    TracePrintf(2, "process_scheduling: Begin schedule_processes");
    struct schedule_item *item = get_head();
    struct process_control_block *current_pcb = item->pcb;

    if(current_pcb->pid != 1 || can_idle_switch()){
        move_head_to_tail();
        select_next_process();
        item = get_head();

        struct process_control_block *next_pcb= item->pcb;
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
    struct schedule_item *current = head;
    struct schedule_item *previous = NULL;

    while(current!= NULL){
        struct process_control_block *pcb = current->pcb;
        if(pcb->delay == delay && pcb->is_waiting == 0 && pcb->is_waiting_to_read_from_terminal == -1
                &&pcb->is_waiting_to_write_to_terminal== -1 && pcb->is_writing_to_terminal == -1){
            if(previous == NULL){
                return 1;
            }else{
                previous->next = current->next;
                current->next = head;
                head = current;
                return 1;
            }
        }else{
            previous = current;
            current = current->next;
        }
    }
    return 0;
}

struct process_control_block * get_pcb_writing_to_terminal(int terminal){
    TracePrintf(3, "process_scheduling: looking for process writing to terminals %d\n", terminal);
    struct schedule_item *current= get_head();

    while(current!=NULL){
        TracePrintf(3, "process_scheduling: current has pid of %d\n", current->pcb->pid);
        TracePrintf(3, "process_scheduling: current is writing to terminal: %d\n", current->pcb->is_writing_to_terminal);
        struct process_control_block *pcb = current->pcb;
        if(pcb->is_writing_to_terminal == terminal){
            return pcb;
        }
        current = current->next;
    }

    TracePrintf(3, "process_scheduling: did not find process writing to terminal %d\n", terminal);
    return NULL;
}

void wake_up_a_writer_for_terminal(int terminal){
    struct schedule_item *current = get_head();
    while(current != NULL){
        struct process_control_block *pcb= current->pcb;
        if(pcb->is_waiting_to_write_to_terminal == terminal){
            pcb->is_waiting_to_write_to_terminal = -1;
            return;
        }
        current = current->next;
    }
    return;
}

void wake_up_a_reader_for_terminal(int terminal){
    struct schedule_item *current = get_head();

    while(current != NULL){
        struct process_control_block *pcb= current->pcb;
        if(pcb->is_waiting_to_read_from_terminal == terminal){
            pcb->is_waiting_to_read_from_terminal = -1;
            return;
        }
        current = current->next;
    }
    return;
}