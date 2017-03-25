#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>


extern int vm_enabled;
void init_pysical_pages(unsigned int);
void occupy_pages(void* lo , void* hi);
void build_kernel_page_table();
unsigned int get_top_page();
int num_free_pages();
void free_page(unsigned int pfn);
int get_free_phy_page();
void* vaddr_to_paddr(void *vm_addr);