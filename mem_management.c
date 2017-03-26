#include "mem_management.h"
#include "page_table_management.h"
#include "process_scheduling.h"
#include "process_control_block.h"

int phy_page_num;
int *phy_page_occupied = NULL;
int vm_enabled = 0;
void *kernel_brk = (void *)VMEM_1_BASE;

int SetKernelBrk(void *addr){
    int i;
    if(vm_enabled){
        int num_page_required= ((long)UP_TO_PAGE(addr) - (long)kernel_brk)/PAGESIZE;
        if(num_free_pages() < num_page_required){
            return -1;
        }else{
            for(i =0;i<num_page_required;i++){
                unsigned int physical_page_number = get_free_phy_page();
                int vpn = ((long)kernel_brk-VMEM_1_BASE)/PAGESIZE + i;

                kernel_page_table[vpn].valid =1;
                kernel_page_table[vpn].pfn = physical_page_number;
            }
        }
    }else{
        if((long)addr <= (long)kernel_brk-PAGESIZE){
            return -1;
        }
        occupy_pages_to(addr);
    }
    return 0;
}
void init_pysical_pages(unsigned int pmem_size){
    //initiate physical page number and an array to keep track of whether a page is occupied.
    phy_page_num = pmem_size/PAGESIZE;
    phy_page_occupied = malloc(phy_page_num* sizeof(int));
    memset(phy_page_occupied,0,phy_page_num);
}

void occupy_pages(void* lo , void* hi) {
    int low = (long) DOWN_TO_PAGE(lo) / PAGESIZE;
    int high = (long) UP_TO_PAGE(hi) / PAGESIZE;
    int i;
    for (i = low; i < high; i++) {
        phy_page_occupied[i] = 1;
    }
}
void occupy_pages_to(void* to) {
    if(phy_page_occupied!=NULL){
        occupy_pages(kernel_brk,to);
    }
    kernel_brk = (void *)UP_TO_PAGE(to);
}

unsigned int get_top_page(){
    unsigned int toppage = DOWN_TO_PAGE(VMEM_1_LIMIT-1);
    int toppn = toppage/PAGESIZE;
    phy_page_occupied[toppn] =1;
    return toppn;
}


int num_free_pages(){
    int count = 0;
    int i;
    for(i=0;i<phy_page_num;i++){
        if(phy_page_occupied[i]==0){
            count++;
        }
    }
    return count;
}

void free_phy_page(unsigned int pfn){
    phy_page_occupied[pfn] = 0;
}

int get_free_phy_page(){
    int i;
    for(i=0;i<phy_page_num;i++){
        if(phy_page_occupied[i] ==0){
            phy_page_occupied[i] = 1;
            return i;
        }
    }
    Halt();
}

void* vaddr_to_paddr(void *vm_addr){
    void *vpage_base = (void*) DOWN_TO_PAGE(vm_addr);
    void *ppage_base;
    int vpn,pfn;

    if(vpage_base >= (void*)VMEM_1_BASE){
        vpn = ((long)(vpage_base - VMEM_1_BASE))/PAGESIZE;
        pfn = kernel_page_table[vpn].pfn;
    }else{
        vpn = (long)vpage_base/PAGESIZE;
        struct schedule_item *current = get_head();
        pfn = current->pcb->page_table[vpn].pfn;
    }
    ppage_base = (void*)(long)(pfn*PAGESIZE);
    long offset = (long)vm_addr&PAGEOFFSET;
    return (void*)((long)ppage_base+offset);
}

void brk_handler(ExceptionInfo *info){
    void *addr = (void*)info->regs[1];
    int i;

    if(UP_TO_PAGE(addr) <= MEM_INVALID_SIZE){
        info->regs[0] = ERROR;
        return;
    }
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    void *brk = pcb->brk;
    void *user_stack_limit = pcb->user_stack_limit;
    struct pte* user_page_table = pcb->page_table;

    if(UP_TO_PAGE(addr) >= DOWN_TO_PAGE(user_stack_limit)-1){
        info->regs[0] = ERROR;
        return;
    }

    if(UP_TO_PAGE(addr)>UP_TO_PAGE(brk)){
        //add more pages
        int num_pages_required = ((long)UP_TO_PAGE(addr)- (long)UP_TO_PAGE(brk))/PAGESIZE;
        if(num_free_pages() < num_pages_required){
            info->regs[0] = ERROR;
            return;
        }else{
            TracePrintf(3,"mem_management: set brk to acquire more pages. ");
            for(i=0;i<num_pages_required;i++){
                unsigned int physical_page_num = get_free_phy_page();
                int vpn = (long)UP_TO_PAGE(brk)/PAGESIZE + i;
                user_page_table[vpn].valid =1;
                user_page_table[vpn].pfn = physical_page_num;
            }
        }
    }else if(UP_TO_PAGE(addr)<UP_TO_PAGE(brk)){
        //free extra pages
        int num_pages_free = ((long)UP_TO_PAGE(brk)-(long)UP_TO_PAGE(addr))/PAGESIZE;
        TracePrintf(3,"mem_management: prepare to free %d pages",num_pages_free);
        for(i=0;i<num_pages_free;i++){
            user_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE-i].valid = 0;
            int pysical_page_num = user_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE -i].pfn;
            free_phy_page(pysical_page_num);
        }
    }
    info->regs[0] =0;
    pcb->brk = (void*)UP_TO_PAGE(addr);
}

void grow_user_stack(void *addr, void *pcb_raw){
    struct process_control_block *pcb = (struct process_control_block *)pcb_raw;
    int i;
    int num_pages_required = (DOWN_TO_PAGE(pcb->user_stack_limit) -DOWN_TO_PAGE(addr))/PAGESIZE;
    for(i=0;i<num_pages_required;i++){
        unsigned int pfn = get_free_phy_page();
        int vpn = (long)DOWN_TO_PAGE(pcb->user_stack_limit)/PAGESIZE - i -1;
        pcb->page_table[vpn].valid = 1;
        pcb->page_table[vpn].pfn = pfn;
    }
    pcb->user_stack_limit = (void*)DOWN_TO_PAGE(addr);
}