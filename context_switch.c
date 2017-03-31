//
// Created by Liu Fang on 3/19/17.
//
#include "context_switch.h"
#include "pcb.h"
#include "mem_management.h"
#include "page_table_management.h"

SavedContext * idle_init_switch(SavedContext *sct, void* p1, void* p2){
    struct process_control_block *pcb1 = (struct process_control_block*)p1;
    struct process_control_block *pcb2 = (struct process_control_block*)p2;

    struct pte* ptb1 = pcb1->page_table;
    struct pte* ptb2 = pcb2->page_table;

    int i=0, temp_vpn =-1;
    for(i= MEM_INVALID_PAGES;i<KERNEL_STACK_BASE/PAGESIZE; i++){
        if(ptb1[i].valid == 0){
            temp_vpn = i;
            break;
        }
    }

    //copy kernel stack
    for(i=0; i< KERNEL_STACK_PAGES; i++){
        unsigned int new_pfn = get_free_phy_page();

        ptb1[temp_vpn].valid = 1;
        ptb1[temp_vpn].kprot = PROT_READ | PROT_WRITE;
        ptb1[temp_vpn].uprot = PROT_READ | PROT_EXEC;
        ptb1[temp_vpn].pfn = new_pfn;

        void *p1_addr = (void*)(long)(((KERNEL_STACK_BASE/PAGESIZE + i) * PAGESIZE)+ VMEM_0_BASE);
        void *temp_addr= (void*)(long)((temp_vpn*PAGESIZE)+ VMEM_0_BASE);

        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);

        memcpy(
                temp_addr,
                p1_addr,
                PAGESIZE
        );

        ptb1[temp_vpn].valid = 0;
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);
        ptb2[i+ KERNEL_STACK_BASE/PAGESIZE].pfn = new_pfn;
    }

    WriteRegister(REG_PTR0, (RCS421RegVal)vaddr_to_paddr(ptb2));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

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
    int i;

    struct process_control_block *pcb1 = (struct process_control_block*)p1;
    struct process_control_block *pcb2 = (struct process_control_block*)p2;

    struct pte* ptb = pcb1->page_table;
    struct pte* ctb = pcb2->page_table;

    int pagecount= (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE ,  copypages =0;
    for(i=0;i< pagecount;i++){
        if(ptb[i].valid == 1) copypages++;
    }


    int temp_vpn = -1;
    for(i= MEM_INVALID_PAGES; i< (pcb1->user_stack_limit-(void *)PAGESIZE)/PAGESIZE;i++){
        if(ptb[i].valid ==0){
            temp_vpn = i;
            break;
        }
    }
    //check space enough?
    if(copypages > num_free_pages()){
        pcb1->out_of_memory =1;
        return &pcb1->saved_context;
    }

    if(temp_vpn!= -1){
        for(i = MEM_INVALID_PAGES; i< pagecount; i++){
            if(ptb[i].valid == 1){
                unsigned int child_pfn = get_free_phy_page();
                ptb[temp_vpn].valid = 1;
                ptb[temp_vpn].kprot = PROT_READ | PROT_WRITE;
                ptb[temp_vpn].uprot = PROT_READ | PROT_EXEC;
                ptb[temp_vpn].pfn = child_pfn;

                void *parent_addr = (void*)(long)((i*PAGESIZE) + VMEM_0_BASE);
                void *temp_addr = (void*)(long)((temp_vpn*PAGESIZE) + VMEM_0_BASE);

                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);
                //copy to child phy page and flush the temp page entry
                memcpy(
                    temp_addr,
                    parent_addr,
                    PAGESIZE
                );

                ptb[temp_vpn].valid = 0;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);

                ctb[i].valid =1;
                ctb[i].pfn = child_pfn;
            }
        }
    }else{
        //no space in region 0, try region 1
        int tmp_vpn_1 = -1;
        for(i=0;i<PAGE_TABLE_LEN;i++){
            if(kernel_page_table[i].valid==0){
                tmp_vpn_1 = i;
                pcb1->out_of_memory = 0;
                break;
            }
        }
        if(tmp_vpn_1 == -1){
            //region 1 also not valid
            pcb1->out_of_memory =1;
            return &pcb1->saved_context;
        }else{

            for(i = MEM_INVALID_PAGES; i< pagecount; i++){
                if(ptb[i].valid == 1){
                    unsigned int child_pfn = get_free_phy_page();

                    kernel_page_table[tmp_vpn_1].valid = 1;
                    kernel_page_table[tmp_vpn_1].pfn = child_pfn;

                    void *parent_addr = (void*)(long)((i*PAGESIZE) + VMEM_0_BASE);
                    void *temp_addr = (void*)(long)((tmp_vpn_1*PAGESIZE) + VMEM_1_BASE);

                    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);
                    //copy to child phy page and flush the temp page entry
                    memcpy(
                            temp_addr,
                            parent_addr,
                            PAGESIZE
                    );

                    kernel_page_table[tmp_vpn_1].valid = 0;
                    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)temp_addr);

                    ctb[i].valid =1;
                    ctb[i].pfn = child_pfn;
                }
            }
        }
    }

    TracePrintf(3, "%p\n", ctb);

    TracePrintf(3, "context_switch: VALID PTE'S:\n");
    for (i = 0; i < VMEM_0_LIMIT/PAGESIZE; i++) {
        if(ctb[i].valid == 1){
            TracePrintf(3, "context_switch: %d, %d \n", i, ctb[i].valid);
        }
    }

    for (i = 0; i < VMEM_0_LIMIT/PAGESIZE; i++) {
        if(ctb[i].valid == 1){
            TracePrintf(3, "context_switch: %d, %d \n", i, ctb[i].valid);
        }
    }

    WriteRegister(REG_PTR0, (RCS421RegVal) vaddr_to_paddr(ctb));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    memcpy(
         &pcb2->saved_context,
         &pcb1->saved_context,
         sizeof(SavedContext)
    );
    return &pcb2->saved_context;
}