//
// Created by Liu Fang on 3/19/17.
//
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>


SavedContext * idle_init_switch(SavedContext *ctxp, void *p1, void *p2);
SavedContext * MyContextSwitch(SavedContext *ctxp, void *p1, void *p2);
SavedContext *init_region_0_for_child(SavedContext *ctxp, void *p1, void *p2);