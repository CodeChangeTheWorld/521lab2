#include "mem_management.h"
#include "page_table_management.h"

struct pte *kernel_page_table;
struct page_table_record *first_page_table_record;

struct page_table_record * get_first_page_table_record(){
    return first_page_table_record;
}
void init_kernel_page_table(){
    int i;
    kernel_page_table = malloc(PAGE_TABLE_SIZE);

    int text_bound = ((long)&_etext - (long)VMEM_1_BASE)/PAGESIZE;
    int heap_bound = ((long)&kernel_brk - (long)VMEM_1_BASE)/PAGESIZE;

    for(i=0;i<PAGE_TABLE_LEN;i++){
        if(i < text_bound){
            kernel_page_table[i].valid = 1;
            kernel_page_table[i].kprot = PROT_READ | PROT_EXEC;
        }else if(i< heap_bound ){
            kernel_page_table[i].valid = 1;
            kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
        }else{
            kernel_page_table[i].valid = 0;
            kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
        }
        kernel_page_table[i].uprot = PROT_NONE;
        kernel_page_table[i].pfn = i+ (long)VMEM_1_BASE/PAGESIZE;
    }
    TracePrintf(2, "page_table_management: kernel page build.");
}

void add_first_record(){
    struct page_table_record *rec = malloc(sizeof(struct page_table_record));
    void * page_base =  (void *)DOWN_TO_PAGE(VMEM_1_LIMIT - 1);

    rec->is_bottom_full =0;
    rec->is_top_full =0;
    rec->page_base = page_base;
    rec->next = NULL;

    unsigned int pfn = get_top_page();
    int vpn = (long)(page_base- VMEM_1_BASE)/PAGESIZE;

    kernel_page_table[vpn].valid = 1;
    kernel_page_table[vpn].pfn = pfn;

    first_page_table_record = rec;
}

void init_initial_page_table(struct pte* page_table){
    TracePrintf(2, "page_table_management: user page table for idle process");
    int i;
    for(i=0;i<PAGE_TABLE_LEN;i++){
        if(i >= KERNEL_STACK_BASE/PAGESIZE){
            page_table[i].valid = 1;
            page_table[i].kprot = PROT_READ | PROT_WRITE;
            page_table[i].uprot = PROT_NONE;
        }else{
            page_table[i].valid = 0; //kernel stack should be occupied
            page_table[i].kprot = PROT_NONE;
            page_table[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
        }
        page_table[i].pfn = i;
    }

    TracePrintf(3, "page_table_management: user page table initialized for idle process");
}

void init_page_table(struct pte* page_table){
    int i;
    for(i=0;i<PAGE_TABLE_LEN;i++){
        if(i >= KERNEL_STACK_BASE/PAGESIZE){
            page_table[i].valid = 1;
            page_table[i].kprot = PROT_READ | PROT_WRITE;
            page_table[i].uprot = PROT_NONE;
        }else{
            page_table[i].valid = 0; //kernel stack should be occupied
            page_table[i].kprot = PROT_NONE;
            page_table[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
        }
    }
    TracePrintf(2,"page_table_management: New page table initialized.\n");
}

struct pte* create_page_table(){
    TracePrintf(3,"page_table_management: create_page_table start");
    struct page_table_record *current = get_first_page_table_record();
    while(current!=NULL){
        if(current->is_top_full == 0){
            struct pte *new_page_table = (struct pte*)((long)current->page_base + PAGE_TABLE_SIZE);
            current->is_top_full = 1;
            return new_page_table;
        }else if(current->is_bottom_full==0){
            struct pte *new_page_table = (struct pte*)current->page_base;
            current->is_bottom_full =1;
            return new_page_table;
        }else{
            current= current->next;
        }
    }
    return create_page_table();
}

void free_page_table(struct pte *page_table){

   void *page_base = (void*)DOWN_TO_PAGE(page_table);
   struct page_table_record *current = get_first_page_table_record();

    //all table record in a pagetable have the same page base.
   while(current!=NULL){
       if(current->page_base == page_base) {
           if((void*)page_table==page_base){
                current->is_bottom_full = 0;
           }else{
                current->is_top_full = 0;
           }
           if(current->is_top_full && current->is_bottom_full && current->next == NULL){
               free_phy_page((long)page_base/PAGESIZE);
               free(current);
           }
           return;
       }
       current=current->next;
   }
   TracePrintf(0,"page_table_management: free_page_table was called on a page table that is not in a page table record!");
   Halt();
}

int num_pages_in_use(struct pte* page_table){
    int i;
    int count =0;
    for(i=0;i<PAGE_TABLE_LEN- KERNEL_STACK_PAGES;i++){
        if(page_table[i].valid == 1){
            count++;
        }
    }
    return count;
}

