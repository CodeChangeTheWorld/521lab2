//
// Created by Liu Fang on 3/19/17.
//
#include "context_switch.h"
#include "process_control_block.h"
#include "mem_management.h"
#include "page_table_management.h"

SavedContext * idle_init_switch(SavedContext *sct, void* p1, void* p2){
    int i=0;
    int j=0;
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
    TracePrintf(1, "context_switch: end kernel stack copy.\n");
    return &pcb1->saved_context;
}

SavedContext *MyContextSwitch(SavedContext *ctxp, void *p1, void *p2){
    TracePrintf(3, "context_switch: Begin context switch");
    struct process_control_block *pcb2 = (struct process_control_block *)p2;

    WriteRegister(REG_PTR0, (RCS421RegVal)vaddr_to_paddr(pcb2->page_table));
    WriteRegister(REG_TLB_FLUSH,(RCS421RegVal)TLB_FLUSH_0);

    return &pcb2->saved_context;
}

SavedContext *init_region_0_for_child(SavedContext *ctxp, void *p1, void *p2){
    int i =0;
    int first_invalid_page = -1;
    struct process_control_block *parent_pcb = (struct process_control_block *)p1;
    struct process_control_block *child_pcb = (struct process_control_block *)p2;

    struct pte* parent_page_table = parent_pcb->page_table;
    struct pte* child_page_table = child_pcb->page_table;

    int num_user_pages = (VMEM_0_LIMIT-VMEM_0_BASE)/PAGESIZE;
    int pages_to_copy = 0;
    for(i=0;i<num_user_pages;i++){
        if(parent_page_table[i].valid == 1){
            pages_to_copy++;
        }
    }

    for(i= MEM_INVALID_PAGES; i< (parent_pcb->user_stack_limit - (void*)PAGESIZE)/PAGESIZE;i++){
        if(parent_page_table[i].valid == 0){
            first_invalid_page = i;
        }
    }

    if(pages_to_copy > num_free_pages()){
        parent_pcb->out_of_memory = 1;
        return &parent_pcb->saved_context;
    }

    //copy region 0 of parent to temp
    if(first_invalid_page!=-1){
        for(i = MEM_INVALID_PAGES;i<num_user_pages;i++){
            if(parent_page_table[i].valid == 1){
                unsigned int child_phy_page_num = get_free_phy_page();

                parent_page_table[first_invalid_page].valid = 1;
                parent_page_table[first_invalid_page].kprot = PROT_READ | PROT_WRITE;
                parent_page_table[first_invalid_page].uprot = PROT_READ | PROT_EXEC;
                parent_page_table[first_invalid_page].pfn = child_phy_page_num;

                void *parent_addr = (void *)(long)((i*PAGESIZE)+VMEM_0_BASE);
                void *temp_addr = (void *)(long)((first_invalid_page*PAGESIZE)+VMEM_0_BASE);
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)first_invalid_page);

                memcpy(
                    temp_addr,
                    parent_addr,
                    PAGESIZE
                );

                parent_page_table[first_invalid_page].valid = 0;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)first_invalid_page);

                child_page_table[i].valid=1;
                child_page_table[i].pfn=child_phy_page_num;
            }
        }
    }else{
        //region 0 is full, put the temp page in region 1
        int first_invalid_page_region_1 = -1;
        for(i=0;i<PAGE_TABLE_LEN;i++){
            if(kernel_page_table[i].valid == 0){
                first_invalid_page_region_1 = i;
                parent_pcb->out_of_memory = 0;
                break;
            }
        }
        if(first_invalid_page_region_1 == -1){
            parent_pcb->out_of_memory = 1;
            return &parent_pcb->saved_context;
        }else{
            for(i=MEM_INVALID_PAGES;i<num_user_pages;i++){
                if(parent_page_table[i].valid ==  1){
                    unsigned int child_phy_page_num = get_free_phy_page();

                    kernel_page_table[first_invalid_page_region_1].valid = 1;
                    kernel_page_table[first_invalid_page_region_1].pfn = child_phy_page_num;

                    void *parent_addr = (void *)(long)((i*PAGESIZE)+VMEM_0_BASE);
                    void *temp_addr = (void *)(long)((first_invalid_page_region_1*PAGESIZE)+VMEM_1_BASE);
                    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)first_invalid_page_region_1);

                    memcpy(
                            temp_addr,
                            parent_addr,
                            PAGESIZE
                    );

                    kernel_page_table[first_invalid_page_region_1].valid = 0;
                    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)first_invalid_page_region_1);

                    child_page_table[i].valid=1;
                    child_page_table[i].pfn=child_phy_page_num;
                }
            }
        }
    }

    WriteRegister(REG_PTR0, (RCS421RegVal)vaddr_to_paddr(child_page_table));
    WriteRegister(TLB_FLUSH_0, (RCS421RegVal) TLB_FLUSH_0);

    memcpy(
       &child_pcb->saved_context,
       &parent_pcb->saved_context,
       sizeof(SavedContext)
    );

    return &child_pcb->saved_context;
}