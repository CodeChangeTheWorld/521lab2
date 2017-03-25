#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "process_scheduling.h"
#include "process_control_block.h"
#include "context_switch.h"
#include "trap_handlers.h"
#include "page_table_management.h"


struct schedule_item *head = NULL;
void add_to_schedule(struct process_control_block* pcb){
    struct schedule_item *item = malloc(sizeof(struct schedule_item));
    item->pcb = pcb;
    item->next = head;
    head = item;
}