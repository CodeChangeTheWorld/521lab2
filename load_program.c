#include "load_program.h"

#include "mem_management.h"
#include "page_table_management.h"
#include "process_scheduling.h"
#include "process_control_block.h"


int
LoadProgram(char *name, char **args, ExceptionInfo *info, struct pte *page_table_to_load)
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char *cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int databss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    //check file can be opened
    if((fd = open(name, O_RDONLY))<0){
        return -1;
    }

    status = LoadInfo(fd,&li);
    switch (status) {
        case LI_SUCCESS:
            break;
        case LI_FORMAT_ERROR:
            TracePrintf(0,
                        "LoadProgram: '%s' not in Yalnix format\n", name);
            close(fd);
            return (-1);
        case LI_OTHER_ERROR:
            TracePrintf(0, "LoadProgram: '%s' other error\n", name);
            close(fd);
            return (-1);
        default:
            TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
            close(fd);
            return (-1);
    }
    size = 0;
    for(i=0; args[i] !=NULL; i++){
        size+= strlen(args[i] +1 );
    }
    argcount = i;
    cp = argbuf = (char*)malloc(size);

    for(i=0;i<args[i]!=NULL;i++){
        strcpy(cp, args[i]);
        cp+= strlen(cp)+1;
    }

    cp = ((char*)USER_STACK_LIMIT)-size;
    cpp = (char**)((unsigned long)cp & (-1 << 4));
    cpp = ((char**)(unsigned long)cpp - ((argcount + 4)* sizeof(void*)));

    text_npg = li.text_size >> PAGESHIFT;
    databss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT-DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    if(MEM_INVALID_PAGES + text_npg + databss_npg + stack_npg + 1 + KERNEL_STACK_PAGES >= PAGE_TABLE_LEN){
        free(argbuf);
        close(fd);
        return (-1);
    }

    int required_free_physical_pages = text_npg + databss_npg + stack_npg -  num_pages_in_use(page_table_to_load);
    if(num_free_pages() < required_free_physical_pages){
        free(argbuf);
        close(fd);
        return -1;
    }

    //initiate stack pointer
    info->sp = (char *)cpp;

    for(i=0;i<PAGE_TABLE_LEN-KERNEL_STACK_PAGES;i++){
        if(page_table_to_load[i].valid == 1){
            free_phy_page(page_table_to_load[i].pfn);
            page_table_to_load[i].valid = 0;
        }
    }

    int text_data_bss_top_boundary = text_npg + databss_npg;
    for(i=0;i<text_data_bss_top_boundary;i++){
        if(i< text_npg){
            page_table_to_load[i+MEM_INVALID_PAGES].valid = 1;
            page_table_to_load[i+MEM_INVALID_PAGES].kprot = PROT_READ | PROT_WRITE;
            page_table_to_load[i+MEM_INVALID_PAGES].uprot = PROT_READ | PROT_EXEC;
            page_table_to_load[i+MEM_INVALID_PAGES].pfn = get_free_phy_page();
        }else if(i< databss_npg + text_npg){
            page_table_to_load[i+MEM_INVALID_PAGES].valid = 1;
            page_table_to_load[i+MEM_INVALID_PAGES].kprot = PROT_READ | PROT_WRITE;
            page_table_to_load[i+MEM_INVALID_PAGES].uprot = PROT_READ | PROT_WRITE;
            page_table_to_load[i+MEM_INVALID_PAGES].pfn = get_free_phy_page();
        }
    }
    struct schedule_item *item = get_head();
    struct process_control_block *pcb = item->pcb;
    pcb->brk = (void*)UP_TO_PAGE((MEM_INVALID_PAGES + text_npg+databss_npg)*PAGESIZE);

    int last_user_page = USER_STACK_LIMIT/PAGESIZE -1;
    for(i=last_user_page;i>last_user_page-stack_npg;i--){
        page_table_to_load[i].valid =1;
        page_table_to_load[i].kprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i].uprot = PROT_READ | PROT_WRITE;
        page_table_to_load[i].pfn = get_free_phy_page();
    }

    pcb->user_stack_limit = (void*)DOWN_TO_PAGE(USER_STACK_LIMIT);
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    if(read(fd, (void*)MEM_INVALID_SIZE,li.text_size + li.data_size) != li.text_size + li.data_size){
        free(argbuf);
        close(fd);
        return -2;
    }
    close(fd);

    for(i = MEM_INVALID_PAGES; i< text_npg+ MEM_INVALID_PAGES;i++){
        page_table_to_load[i].kprot = PROT_READ | PROT_EXEC;
    }
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    memset((void*)(MEM_INVALID_SIZE + li.text_size + li.data_size),'\0', li.bss_size);

    //set entry point in the exception frame
    info->pc = (void*)li.entry;
    *cpp++ = (char*)argcount;
    cp2 = argbuf;
    for(i=0;i<argcount;i++){
        *cpp++ = cp;
        strcpy(cp,cp2);
        cp += strlen(cp)+1;
        cp2 += strlen(cp2)+1;
    }
    free(argbuf);
    *cpp++ = NULL;
    *cpp++ = NULL;
    *cpp++ = 0;

    for(i=0;i<NUM_REGS;i++){
        info->regs[i] = 0;
    }
    info->psr =0;
    return (0);
}
