#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "trap_handlers.h"
#include "mem_management.h"
#include "page_table_management.h"
#include "load_program.h"
#include "context_switch.h"
#include "process_control_block.h"
#include "process_scheduling.h"
#include "terminals.h"


void **interrupt_table;
int is_init = 1;
void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void* orig_brk, char **cmd_args){
    TracePrintf(1,"Kernel Start, called %d pages.\n", pmem_size/PAGESIZE);
    int i;
    //Occupy pages for kernel stack
    init_pysical_pages(pmem_size);
    occupy_pages((void*)KERNEL_STACK_BASE, (void*)KERNEL_STACK_LIMIT);

    //build interupt table
    interrupt_table = malloc(TRAP_VECTOR_SIZE * sizeof(void*));
    for(i=0;i<TRAP_VECTOR_SIZE;i++){
        switch(i){
            case TRAP_KERNEL:
                interrupt_table[i] = kernel_trap_handler;
                break;
            case TRAP_CLOCK:
                interrupt_table[i] = clock_trap_handler;
                break;
            case TRAP_ILLEGAL:
                interrupt_table[i] = illegal_trap_handler;
                break;
            case TRAP_MEMORY:
                interrupt_table[i] = memory_trap_handler;
                break;
            case TRAP_MATH:
                interrupt_table[i] = math_trap_handler;
                break;
            case TRAP_TTY_RECEIVE:
                interrupt_table[i] = tty_recieve_trap_handler;
                break;
            case TRAP_TTY_TRANSMIT:
                interrupt_table[i] = tty_transmit_trap_handler;
                break;
            default:
                interrupt_table[i] = NULL;

        }
    }

    //write REG_VECTOR_BASE
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)interrupt_table);

    //kernel page table
    build_kernel_page_table();
    add_first_record();

    TracePrintf(2,"kernel_start: added first page table kernel");

    //Initiate user page table, needs to build process
    struct process_control_block *pcb=create_idle_process();

    //Write page table register and enable virtual memory
    WriteRegister(REG_PTR0, (RCS421RegVal)pcb->page_table);
    WriteRegister(REG_PTR1, (RCS421RegVal)kernel_page_table);
    WriteRegister(REG_VM_ENABLE, 1);
    vm_enabled = 1;

    //page table record can only be made after vm enabled.
    struct process_control_block *pcb=create_new_process(INIT_PID, ORPHAN_PARENT_PID);

    char *loadargs[1];
    loadargs[0] = NULL;
    LoadProgram("idle",loadargs, info, idle_pcb->page_table);

    ContextSwitch(idle_and_init_initialization,&idle_pcb->saved_context , (void*)idle_pcb, (void*)init_pcb);
    if(is_init==1){
        is_init=0;
        if(cmd_args[0] == NULL){
            LoadProgram("init",loadargs,info,init_pcb->page_table);
        }else{
            LoadProgram(cmd_args[0],cmd_args,info,init_pcb->page_table);
        }
        init_charbuffers();
    }
}

