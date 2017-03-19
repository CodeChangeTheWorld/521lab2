
int *is_page_occupied = NULL;
int vm_enabled = 0;
int pagenum;

void
init_physical_mem_arr(pmem_size){
    int pagenum= pmem_size/PAGESIZE;
    is_page_occupied = malloc(pagenum* sizeof(int));
    memset(is_page_occupied,0,pagenum);
}

void
occupy_pages(void *begin,void *end){
    int beginpage = (long)DOWN_TO_PAGE(begin)/PAGESIZE;
    int endpage = (long)UP_TO_PAGE(end)/PAGESIZE;

    for(int i=beginpage;i<endpage;i++){
        is_page_occupied = 1;
    }
}