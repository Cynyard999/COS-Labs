
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	// 循环，直到找到下一个没有waiting并且没有睡觉的进程
	while (TRUE)
	{
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS){
			p_proc_ready = proc_table;
		}
		if (p_proc_ready->sleep_ticks == 0 && p_proc_ready->is_waiting == FALSE){
			break;
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}



/*======================================================================*
                           sys_milli_sleep
 *======================================================================*/
PUBLIC void sys_milli_sleep(int mill_seconds){
	p_proc_ready->sleep_ticks = mill_seconds*HZ/1000;
	schedule();

}



/*======================================================================*
                           sys_print
 *======================================================================*/

PUBLIC void sys_print(char *str){
	disp_str(str);
}


/*======================================================================*
                           sys_print_color
 *======================================================================*/

PUBLIC void sys_print_color(char *str,int color){
	disp_color_str(str,color);
}

/*======================================================================*
                           sys_sem_p
 *======================================================================*/

PUBLIC void sys_sem_p(SEMAPHORE *s){
	s->value--;
	if (s->value<0)
	{
		p_proc_ready->is_waiting = TRUE;
		s->queue[s->queue_end] = p_proc_ready;
		s->queue_end = (s->queue_end+1)%MAX_WAITING_PROCESS_QUEUE_SIZE;
		schedule();
	}
}


/*======================================================================*
                           sys_sem_v
 *======================================================================*/

PUBLIC void sys_sem_v(SEMAPHORE *s){
	s->value++;
	if (s->value<=0)
	{
		s->queue[s->queue_start]->is_waiting = FALSE;
		p_proc_ready = s->queue[s->queue_start];
		s->queue[s->queue_start] = NULL;
		s->queue_start = (s->queue_start+1)%MAX_WAITING_PROCESS_QUEUE_SIZE;
	}
}