#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>


extern int vm_enabled;
extern void *kernel_brk;


void init_pysical_pages(unsigned int);
void occupy_pages(void* lo , void* hi);
unsigned int get_top_page();
int num_free_pages();
void free_phy_page(unsigned int pfn);
int get_free_phy_page();
void* vaddr_to_paddr(void *vm_addr);
void brk_handler(ExceptionInfo *info);
void occupy_pages_to(void* to);
int SetKernelBrk(void *addr);
void grow_user_stack(void *addr, void *pcb_raw);