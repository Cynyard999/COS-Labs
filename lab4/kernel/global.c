
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

PUBLIC	TASK	task_table[NR_TASKS] = {{Reader_A, STACK_SIZE_READERA, "Reader_A"},
					{Reader_B, STACK_SIZE_READERB, "Reader_B"},
					{Reader_C, STACK_SIZE_READERC, "Reader_C"},
                    {Writer_D, STACK_SIZE_WRITERD, "Writer_D"},
                    {Writer_E, STACK_SIZE_WRITERE, "Writer_E"},
                    {Notifier_F, STACK_SIZE_NOTIFIERF, "Notifier_F"},};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks, sys_milli_sleep ,sys_print,sys_print_color,sys_sem_p,sys_sem_v};

