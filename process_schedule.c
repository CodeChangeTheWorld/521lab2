//
// Created by Liu Fang on 3/19/17.
//


int next_pid = BASE_PID;
struct schedule_item *head = NULL;


//after current process run for 2 ticks,move it to last and make its next as head, context switch
void
schedule_processes(){
    struct schedule_item *item = get_head();
    struct process_control_block *current_pcb = item->pcb;

    if(current_pcb->pid != 1 || can_idle_switch()){
        move_head_to_tail();
        select_next_process();

        item = get_head();
        struct process_control_block *next_pcb = item->pcb;

        ContextSwitch(context_switch_helper,&current_pcb->saved_context, (void*) current_pcb, (void*)next_pcb);
        reset_time_till_switch();
    }
}

void
get_current_pid(){
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    return pcb->pid;
}

int
get_next_pid(){
    return next_pid++;
}

struct schedule_item *
get_head(){
    return head;
}

void
move_head_to_tail(){
    if(head->next == NULL){
        return;
    }
    if(head != NULL){
        struct schedule_item *current = head;
        struct schedule_item *new_head = head->next;
        while(current->next != NULL){
            current = current->next;
        }
        head->next = NULL;
        current->next = head;
        head = new_head;
    }
}

int
move_next_process_to_head(int delay){
    struct schedule_item *current = head;
    struct schedule_item *previous = NULL;

    while(current=NULL){
        struct process_control_block *pcb = current->pcb;
        if(pcb->delay == delay
           && pcb->is_waiting == 0
           && pcb->is_waiting_to_read_from_terminal == -1
           && pcb->is_waiting_to_write_to_terminal == -1
           && pcb->is_writing_to_terminal == -1){
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

//select next item who's pcb->delay is 0, assume move_head_to_tail() has been called
void
select_next_process(){
    if(move_next_process_to_head(0)){
        return;
    }else if(move_next_process_to_head(IDLE_DELAY)){
        return;
    }
    Halt(); // Error happend.
}

int
can_idle_switch(){
    struct schedule_item *current = head->next;
    while(current != NULL){
        struct process_control_block *pcb = current->pcb;
        if(pcb->delay == 0){
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void
schedule_processes_during_decapitate(){
    //If idle at the head, make sure that there is sth to switch to
    struct schedule_item *old_head = get_head();
    struct process_control_block *old_head_pcb = old_head->pcb;

    head = old_head->next;
    select_next_process();

    struct schedule_item *next_head = get_head();
    struct process_control_block *next_pcb = next_head->pcb;

    ContextSwitch(context_switch_helper, &old_head_pcb->saved_context, (void *)old_head_pcb,(void *)next_pcb);
    reset_time_till_switch();
}

void
decapitate(){
  struct schedule_item *current = get_head();
    if(current ==NULL){
        Halt();
    }
    struct process_control_block *current_pcb = current->pcb;
    if(current_pcb->pid == IDLE_PID){
        Halt();
    }
    schedule_processes_during_decapitate();

    raw_remove_head_of_schedule();
}

void
raw_remove_head_of_schedule(){
    struct schedule_item *current = get_head();
    head = current->next;

    free_page_table(current->pcb->page_table);
    free(current->pcb);
    free(current);
}

void
add_pcb_to_schedule(struct process_control_block *pcb){
    struct schedule_item *new_item = malloc(sizeof(struct schedule_item));
}
