#include "mem_management.h"
#include "page_table_management.h"

struct pte *kernel_page_table;
struct page_table_record *first_page_table_record;

void
init_kernel_page_table() {
    int end_of_text = ((long)&_etext -(long)VMEM_1_BASE) / PAGESIZE;
    int end_of_heap = ((long)&kernel_brk -(long)VMEM_1_BASE) / PAGESIZE;

    for(int i=0;i<PAGE_TABLE_LEN;i++){
        if(i<end_of_text){
            kernel_page_table[i].valid=1;
            kernel_page_table[i].kprot = PROT_READ | PROT_EXEC;
        }else if(i<=end_of_heap){
            kernel_page_table[i].valid=1;
            kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
        }else{
            kernel_page_table[i].valid=0;
            kernel_page_table[i].kprot = PROT_READ | PROT_WRITE;
        }
        kernel_page_table[i].uprot = PROT_NONE;
        kernel_page_table[i].pfn = i + (long)VMEM_1_BASE/PAGESIZE;
    }
}

void
init_first_page_table_record(){
    struct page_table_record *first_record= malloc(sizeof(page_table_record));
    void *base= (void*) DOWN_TO_PAGE(VMEM_1_LIMIT -1);

    first_record->page_base = base;
    first_record->is_top_full = 0;
    first_record->is_bottom_full = 0;
    first_record->next = NULL;

    unsigned int pfn = acquire_top_physical_page();
    int vpn = (long)(base -VMEM_1_BASE)/PAGESIZE;

    kernel_page_table[vpn].valid = 1;
    kernel_page_table[vpn].pfn =pfn;

    first_page_table_record=first_record;
}
