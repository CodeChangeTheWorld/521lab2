#include "context_switch.h"
#include "process_control_block.h"
#include "mem_management.h"
#include "page_table_management.h"

SavedContext * idle_init_switch(SavedContext *sct, void* p1, void* p2){
    int i;
    int j;
    struct process_control_block *pcb1 = (struct process_control_block*)p1;
    struct process_control_block *pcb2 = (struct process_control_block*)p2;

    struct pte *p1_page_table = pcb1->page_table;
    struct pte *p2_page_table = pcb2->page_table;

    for(i=0;i<KERNEL_STACK_PAGES;i++){
        unsigned int p2_phy_page_num = get_free_phy_page();
        for(j=MEM_INVALID_PAGES;j<KERNEL_STACK_BASE/PAGESIZE;j++){
            if(p1_page_table[j].valid == 0){
                p1_page_table[j].valid = 1;
                p1_page_table[j].kprot = PROT_READ | PROT_WRITE;
                p1_page_table[j].uprot = PROT_READ | PROT_EXEC;
                p1_page_table[j].pfn = p2_phy_page_num;

                void *p1_vaddr = (void*)(long)(((KERNEL_STACK_BASE/PAGESIZE + i)*PAGESIZE)+ VMEM_0_BASE);
                void *temp_va_for_kernel_stack = (void*)(long)((j*PAGESIZE) + VMEM_0_BASE);
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_va_for_kernel_stack );

                memcpy(
                        temp_va_for_kernel_stack,
                        p1_vaddr,
                        PAGESIZE
                );

                p1_page_table[j].valid =0 ;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_va_for_kernel_stack);

                p2_page_table[i+KERNEL_STACK_BASE/PAGESIZE].pfn = p2_phy_page_num;
                break;
            }
        }
    }

    WriteRegister(REG_PTR0, (RCS421RegVal)vaddr_to_paddr(pcb2->page_table));
    WriteRegister(REG_TLB_FLUSH,(RCS421RegVal)TLB_FLUSH_0);
    return &pcb1->saved_context;
}

SavedContext *MyContextSwitch(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(3, "context_switch: Begin context switch");
    struct process_control_block *pcb2 = (struct process_control_block *)p2;

    WriteRegister(REG_PTR0, (RCS421RegVal)vaddr_to_paddr(pcb2->page_table));
    WriteRegister(REG_TLB_FLUSH,(RCS421RegVal)TLB_FLUSH_0);

    return &pcb2->saved_context;
}