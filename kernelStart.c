#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "trapHandlers.h"
#include "memManagement.h"
#include "pageTableManagement.h"
#include "load_program.h"
#include "contextSwitch.h"
#include "pcb.h"
#include "processQueue.h"
#include "terminals.h"


void **interrupt_table;
int is_init = 0;

/*
 *  This is the primary entry point into the kernel:
 *
 *  ExceptionInfo *info Pointer to an initial ExceptionInfo structure
 *  unsigned int pmem_size The total size of the physical memory of the machine
 *  void *orig_brk The initial value of the kernel’s "break". The first address that is not part of the kernel’s initial heap.
 *  char **cmd_args A vector of strings containing a pointer to each argument from the boot command line.
 */
void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
    
    int i;
    //Occupy pages for kernel stack
    TracePrintf(0,"kernel_start: Initialize memory with %d bytes.\n", pmem_size);
    init_physical_pages(pmem_size);
    TracePrintf(0,"kernel_start: called %d pages.\n", pmem_size/PAGESIZE);
    occupy_pages((void*)KERNEL_STACK_BASE, (void*)KERNEL_STACK_LIMIT);

    //build interupt table and write to register
    interrupt_table = malloc(TRAP_VECTOR_SIZE * sizeof(void*));
    interrupt_table[TRAP_KERNEL] = kernel_trap_handler;
    interrupt_table[TRAP_CLOCK] = clock_trap_handler;
    interrupt_table[TRAP_ILLEGAL] = illegal_trap_handler;
    interrupt_table[TRAP_MEMORY] = memory_trap_handler;
    interrupt_table[TRAP_MATH] = math_trap_handler;
    interrupt_table[TRAP_TTY_RECEIVE] = tty_recieve_trap_handler;
    interrupt_table[TRAP_TTY_TRANSMIT] = tty_transmit_trap_handler;
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal)interrupt_table);

    //kernel page table
    init_kernel_page_table();
    add_first_record();
    TracePrintf(0,"kernel_start: added first page table kernel");

    //Initiate user page table, needs to build a idel process
    ProcessControlBlock *idle_pcb = create_idle_process();

    //Write page table register and enable vitual memory
    WriteRegister(REG_PTR0, (RCS421RegVal) idle_pcb->page_table);
    WriteRegister(REG_PTR1, (RCS421RegVal) kernel_page_table);
    TracePrintf(0, "kernel_start: Kernel and user page table ready.\n");
    WriteRegister(REG_VM_ENABLE, 1);
    vm_enabled = 1;
    TracePrintf(0, "kernel_start: Virturl memory enabled.\n");

    //Page table record can only be made after vm enabled.
    TracePrintf(0, "kernel_start: Create new process.\n");
    ProcessControlBlock *init_pcb = create_new_process(INIT_PID, ORPHAN_PARENT_PID);

    //Load Process
    char *loadargs[1];
    loadargs[0] = NULL;
    LoadProgram("idle", loadargs, info, idle_pcb->page_table);
    TracePrintf(0, "kernel_start: load idle process.\n");
    ContextSwitch(idle_init_switch, &idle_pcb->saved_context , (void*)idle_pcb, (void*)init_pcb);

    if(is_init == 0){
        is_init = 1;
        if(cmd_args[0] == NULL){
            LoadProgram("init", loadargs, info, init_pcb->page_table);
        }else{
            LoadProgram(cmd_args[0], cmd_args, info, init_pcb->page_table);
        }
        init_charbuffers();
    }
}

