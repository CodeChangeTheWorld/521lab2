#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "mem_management.h"
#include "page_table_management.h"
#include "load_program.h"
#include "context_switch.h"
#include "process_control_block.h"


void ** interupt_vector_table;
int is_init = 1;

void KernelStart(ExceptionStackFrame *frame, unsigned int pmem_size, void* orig_brk, char **cmd_args){
    //init structrure to keep track of free pages
    init_physical_mem_arr(pmem_size);

    //occupy kernel stack
    occupy_pages((void*) KERNEL_STACK_BASE, (void*)KERNEL_STACK_LIMIT);

    //build interupt vector table
    interupt_vector_table= malloc(TRAP_VECTOR_SIZE * sizeof(void *));
    for(int i=0;i<TRAP_VECTOR_SIZE;i++){
        switch(i){
            case TRAP_KERNEL:
                interupt_vector_table[i] = kernel_trap_handler;
                break;
            case TRAP_CLOCK:
                interupt_vector_table[i] = clock_trap_handler;
                break;
            case TRAP_ILLEGAL:
                interupt_vector_table[i] = illegal_trap_handler;
                break;
            case TRAP_MEMORY:
                interupt_vector_table[i] = memory_trap_handler;
                break;
            case TRAP_MATH:
                interupt_vector_table[i] = math_trap_handler;
                break;
            case TRAP_TTY_RECEIVE:
                interupt_vector_table[i] = tty_receive_trap_handler;
                break;
            case TRAP_TTY_TRANSMIT:
                interupt_vector_table[i] = tty_transmit_trap_handler;
                break;
            default:
                interupt_vector_table[i] = NULL;
        }
    }
    //Initiate REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)interupt_vector_table);

    //Initiate Kernel page table
    init_kernel_page_table();
    init_first_page_table_record();

    //init region0,region1
    struct process_control_block *idle_pcb = create_idle_process();
    WriteRegister(REG_PTR0,(RCS421RegVal)idle_pcb->page_table);
    WriteRegister(REG_PTR1,(RCS421RegVal)kernel_page_table);

    //enable virtual memory
    WriteRegister(REG_VM_ENABLE,1);
    vm_enabled = 1;

    struct process_control_block *init_pcb = create_new_process(INIT_PID,ORPHAN_PARENT_PID);
    char *loadargs[1];
    loadargs[0] = NULL;
    LoadProgram("idle",loadargs,frame,idle_pcb->page_table);

    ContextSwitch(idle_and_init_initialization, &idle_pcb->saved_context,(void *)idle_pcb, (void *)init_pcb);
    if(is_init == 1){
        is_init = 0;
        if(cmd_args[0] == NULL){
            LoadProgram("init", loadargs, frame, init_pcb->page_table);
        } else {
            LoadProgram(cmd_args[0], cmd_args, frame, init_pcb->page_table);
        }
        init_charbuffers();
    }
}