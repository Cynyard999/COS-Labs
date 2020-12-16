
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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
                            global_variables
 *======================================================================*/
SEMAPHORE *rc_mutex;
SEMAPHORE *wc_mutex;
SEMAPHORE *rw_mutex;
SEMAPHORE *max_r_mutex;
SEMAPHORE *reader_queue;
SEMAPHORE *readers_queue;
SEMAPHORE *writers_queue; 
int MAX_READER_NUM;
int reader_count;
int writer_count;
int WRITE_FIRST;

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");
	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	// 初始化proc_table
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		initProcess(p_proc);

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	init();

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

	

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
	out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

	while(1){}
}

/*======================================================================*
                            init
 *======================================================================*/
void init(){
	WRITE_FIRST = FALSE;
	MAX_READER_NUM = 3;
	clean();
	initSemaphore();
}

/*======================================================================*
                            clean
 *======================================================================*/
void clean(){
	disp_pos = 0;
	for (int i = 0; i < 80*25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
}

/*======================================================================*
                            initSemaphore
 *======================================================================*/
void initSemaphore(){
	reader_count =0;
	writer_count =0;
	rc_mutex = &reader_count_mutex;
	wc_mutex = &writer_count_mutex;
	rw_mutex = &read_write_mutex;
	max_r_mutex = &max_reader_mutex;
	reader_queue = &reader_queue_mutex;
	readers_queue = &readers_queue_mutex;
	writers_queue = &writers_queue_mutex;

	// reader_count一次只能有一个reader访问
	rc_mutex->value = 1;
	rc_mutex->queue_start = 0;
	rc_mutex->queue_end = 0;
	// writer_count一次只能有一个reader访问
	wc_mutex->value = 1;
	wc_mutex->queue_start = 0;
	wc_mutex->queue_end = 0;
	// 只有一本书能给读者写者使用
	rw_mutex->value = 1;
	rw_mutex->queue_start = 0;
	rw_mutex->queue_end = 0;
	// 同时最多n个人读这本书
	max_r_mutex->value = MAX_READER_NUM;
	max_r_mutex->queue_start = 0;
	max_r_mutex->queue_end = 0;
	// 写优先的读者队列信号量
	writers_queue->value = 1;
	writers_queue->queue_start = 0;
	writers_queue->queue_end = 0;
	// 读优先的写者队列信号量
	reader_queue->value = 1;
	reader_queue->queue_start = 0;
	reader_queue->queue_end = 0;
	readers_queue->value = 1;
	readers_queue->queue_start = 0;
	readers_queue->queue_end = 0;
}



/*======================================================================*
                            initProcess
 *======================================================================*/
void initProcess(PROCESS * p){
	p->is_waiting = FALSE;
	p->sleep_ticks = 0;
}


/*======================================================================*
                            processReady
 *======================================================================*/
void processReady(int pid){
	char info[20] = "A Ready|";
	info[0] = info[0]+pid;
	print_color(info,pid+1);
}


/*======================================================================*
                            processing
 *======================================================================*/
void processing(int pid){
	char info[20] = "A ";
	info[0] = info[0]+pid;
	if (pid<3)
	{
		strcpy(info + 2, "Reading|");
	}
	else
	{
		strcpy(info + 2, "Writing|");
	}
	print_color(info,pid+1);
}


/*======================================================================*
                            processOver
 *======================================================================*/
void processOver(int pid){
	char info[20] = "A ";
	info[0] = info[0]+pid;
	if (pid<3)
	{
		strcpy(info + 2, "ReadingOver|");
	}
	else
	{
		strcpy(info + 2, "WritingOver|");
	}
	print_color(info,pid+1);
}



/*======================================================================*
                            notify
 *======================================================================*/
void notify(int readerNum){
	if (readerNum == -1)
	{
		char info[] = "1 WRITER|";
		print(info);
	}
	else
	{
		char info[] = "0 READER(S)|";
		info[0] = info[0]+readerNum;
		print(info);
	}
}

/*======================================================================*
                               Reading_Process
 *======================================================================*/
void Reading_Process(int pid, int time_slice){
	while (1) {
		processReady(pid);
		P(max_r_mutex);
		if (WRITE_FIRST)
		{
			// 如果有进程已经申请了写，后续的第一个读请求就会堵塞在reader_queue，其他的读请求就会堵塞在readers_queue
			// readers_queue的作用是让reader_queue最多只有一个reader排队，后来的读请求就可以插队到reader_queue里面
			P(readers_queue);
				P(reader_queue);
					P(rc_mutex);
					reader_count++;
					if (reader_count == 1)
					{
						// 读过程中禁止写, 而其他读的进程不会被堵塞
						P(rw_mutex);
					}
					V(rc_mutex);
				V(reader_queue);
			V(readers_queue);
		}
		else
		{
			P(rc_mutex);
			reader_count++;
			if (reader_count == 1)
			{
				// 读过程中禁止写, 而其他读的进程不会被堵塞
				P(rw_mutex);
			}
			V(rc_mutex);
		}

		// begin reading
		processing(pid);
		milli_delay(time_slice*MS_PER_TIME_SLICE);
		// finish reading
		processOver(pid);

		P(rc_mutex);
		reader_count--;
		if (reader_count == 0)
		{
			V(rw_mutex);
		}
		V(rc_mutex);

		V(max_r_mutex);
		// 一定程度上防止饿死，让进程执行完后休息一下（防累死）
		//milli_delay(2*MS_PER_TIME_SLICE);
		
	}
}

void Writing_Process(int pid, int time_slice){
	while(1){
		processReady(pid);
		if (WRITE_FIRST)
		{
			// 有读/写进程正在使用共享文件时，后续写请求访问，插队，插入到第一个读请求前面
			P(wc_mutex);
			writer_count++;
			if (writer_count == 1)
			{
				P(reader_queue);
			}
			V(wc_mutex);
		}
		else
		{
			// 有读进程正在使用共享文件时，reader的reader_count中逻辑已经决定了后续读请求不会受到堵塞，后续的写进程会堵塞在rw_mutex和writers_queue
			// 有写进程正在使用共享文件时，后续写的请求会堵在writers_queue，直到当前进程写完了，才会进入rw_mutex堵塞；而后续读的请求会直接在rw_mutex堵塞
			P(wc_mutex);
			writer_count++;
			V(wc_mutex);
			P(writers_queue);
		}
		
		P(rw_mutex);
		processing(pid);
		milli_delay(time_slice*MS_PER_TIME_SLICE);
		processOver(pid);
		V(rw_mutex);

		if (WRITE_FIRST)
		{
			P(wc_mutex);
			writer_count--;
			if (writer_count == 0)
			{
				V(reader_queue);
			}
			V(wc_mutex);
		}
		else
		{
			P(wc_mutex);
			writer_count--;
			V(wc_mutex);
			V(writers_queue);
		}

		// 一定程度上防止饿死，让进程执行完后休息一下（防累死）
		// milli_delay(3*MS_PER_TIME_SLICE);
	}
}

/*======================================================================*
                               Reader_A
 *======================================================================*/
void Reader_A()
{
	Reading_Process(0,2);
}

/*======================================================================*
                               Reader_B
 *======================================================================*/
void Reader_B()
{
	Reading_Process(1,3);
}

/*======================================================================*
                               Reader_C
 *======================================================================*/
void Reader_C()
{
	Reading_Process(2,3);
}

/*======================================================================*
                               Writer_D
 *======================================================================*/
void Writer_D()
{
	Writing_Process(3,3);
}

/*======================================================================*
                               Writer_E
 *======================================================================*/
void Writer_E()
{
	Writing_Process(4,4);	
}

/*======================================================================*
                               Notifier_F
 *======================================================================*/
void Notifier_F()
{
	while(1){
		if (reader_count == 0 && rw_mutex->value != 1)
		{
			notify(-1);
		}
		else if (reader_count > 0)
		{
			notify(reader_count);
		}
		milli_sleep(1*MS_PER_TIME_SLICE);
	}
}