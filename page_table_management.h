#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct page_table_record{
    void *page_base;
    int is_top_full;
    int is_bottom_full;
    struct page_table_record *next;
};
extern struct pte *kernel_page_table;
void build_kernel_page_table();
void add_first_record();
void init_new_page_table(struct pte* page_table);
void init_idle_page_table(struct pte* page_table);
