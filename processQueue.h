#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

#define INIT_PID 0
#define IDLE_PID 1
#define BASE_PID 2
#define IDLE_DELAY -1


int get_current_pid();
int get_next_pid();
void move_head_to_tail();
//void

void add_to_schedule(struct process_control_block* pcb);
struct process_control_block * get_head();
int is_current_process_orphan();
struct process_control_block *get_pcb_by_pid(int pid);

void decapitate();
void schedule_process_during_decap();
void select_next_process();
void move_head_to_tail();
void wake_up_a_writer_for_terminal(int terminal);
void wake_up_a_reader_for_terminal(int terminal);
