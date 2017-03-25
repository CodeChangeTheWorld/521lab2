#include "mem_management.h"
#include "page_table_management.h"
#include "process_scheduling.h"
#include "process_control_block.h"

int phy_page_num;
int *phy_page_occupied = NULL;
int vm_enabled = 0;
void *kernel_brk = (void *)VMEM_1_BASE;

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