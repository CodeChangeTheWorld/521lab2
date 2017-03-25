#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#define ORPHAN_PARENT_PID -1

struct exit_status_node{
    int exit_status;
    int pid;
    struct exit_status_node *next;
};
struct process_control_block{
    int pid;
    struct pte *page_table;
    SavedContext saved_context;
    int delay;
    void *brk;
    void *user_stack_limit;
    struct exit_status_node *exit_status_queue;
    int parent_pid;
    int out_of_memory;
    int is_waiting;
    int num_children;
    int is_waiting_to_read_from_terminal;
    int is_writing_to_terminal;
    int is_waiting_to_write_to_terminal;
};

struct process_control_block *create_new_process(int pid, int parent_id);
struct process_control_block * create_idle_process();
struct process_control_block* create_empty_process(int pid, int parent_pid);