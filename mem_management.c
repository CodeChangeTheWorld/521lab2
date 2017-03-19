
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

void
brk_handler(ExceptionStackFrame *frame){
    void *addr = (void *)frame->regs[1];

    if(UP_TO_PAGE(addr)<=MEM_INVALID_SIZE){
        frame->regs[0] = ERROR;
        return;
    }
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    void *brk = pcb->brk;
    void *user_stack_limit = pcb->user_stack_limit;
    struct pte *use_page_table = pcb->page_table;

    if(UP_TO_PAGE(addr) >= DOWN_TO_PAGE(user_stack_limit) -1){
        frame->regs[0] = ERROR;
        return;
    }

    if(UP_TO_PAGE(addr)> UP_TO_PAGE(brk)){
        int num_pages_required = ((long)UP_TO_PAGE(addr)-(long)UP_TO_PAGE(brk))/PAGESIZE;
        if(num_free_physical_pages() < num_pages_required){
            frame->regs[0] = ERROR;
            return;
        }else{
            for(int i=0;i<num_pages_required; i++){
                unsigned int physical_page_number = accquire_free_physical_page();
                int vpn = (long)UP_TO_PAGE(brk)/PAGESIZE + i;
                use_page_table[vpn].valid =1;
                use_page_table[vpn].pfn = physical_page_number;
            }
        }
    }else if(UP_TO_PAGE(addr) < UP_TO_PAGE(brk)){
        //free memory
        int num_pages_to_free = ((long)UP_TO_PAGE(brk)-(long)UP_TO_PAGE(addr))/PAGESIZE;
        for(int i=0;i<num_pages_to_free;i++){
            use_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE -i].valid = 0;
            int physical_page_number = user_page_table[(long)UP_TO_PAGE(brk)/PAGESIZE -i].pfn;
            free_physical_page(physical_page_number);
        }
    }
    frame->regs[0] = 0;
    pcb->brk = (void*)UP_TO_PAGE(addr);

}