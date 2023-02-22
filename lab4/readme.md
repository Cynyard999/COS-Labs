## 添加系统调用

#### syscall.asm
添加中断调用号

```
_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应
_NR_milli_sleep 	equ 1 ;
_NR_print			equ 2 ;
_NR_print_color		equ 3 ;
_NR_P				equ 4 ;
_NR_V				equ 5 ;
```

添加函数代码

```
print_color:
	mov	eax, _NR_print_color
	mov ecx, [esp+8]
	mov ebx, [esp+4]
	int	INT_VECTOR_SYS_CALL
	ret
```

#### proc.c

添加系统调用函数代码

```c
...
PUBLIC void sys_print_color(char *str,int color){
	disp_color_str(str,color);
}
...
```

#### global.c

中添加系统调用函数名

```c
PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = 
{sys_get_ticks, sys_milli_sleep ,sys_print,sys_print_color,sys_sem_p,sys_sem_v};
```

#### kernel.asm

修改sys_call，匹配函数的参数个数

```
sys_call:
        call    save
				;push	dword[p_proc_ready]
        sti
		
				; 最多的时候只多了两个参数 这里相当于sys_fun的最多的参数
				push	ecx
				push	ebx
        
        call    [sys_call_table + eax * 4]
        
				add 	esp, 4*2 ;栈指针复位

        mov     [esi + EAXREG - P_STACKBASE], eax

        cli

        ret
```

#### proto.h

添加函数声明

```c
...
PUBLIC void     sys_milli_sleep(int mills);
PUBLIC void     sys_print(char * str);
PUBLIC  void    sys_print_color(char * str, int color);
PUBLIC void     sys_sem_p(SEMAPHORE *s);
PUBLIC void     sys_sem_v(SEMAPHORE *s);
...
PUBLIC  void    milli_sleep(int mills);
PUBLIC  void    print(char * str);
PUBLIC  void    print_color(char * str, int color);
PUBLIC  void    P(SEMAPHORE *s);
PUBLIC  void    V(SEMAPHORE *s);
```

#### syscall.asm

导出函数声明

```
global	get_ticks
global	milli_sleep
global  print
global	print_color
global 	P
global 	V
```

之后就可以在函数中使用syscall导出的函数

## 读者写者问题

### 创建进程

#### proc.h

修改进程数量和创建进程的栈空间

```c
/* Number of tasks */
#define NR_TASKS	6

/* stacks of tasks */
#define STACK_SIZE_READERA	0x8000
#define STACK_SIZE_READERB	0x8000
#define STACK_SIZE_READERC	0x8000
#define STACK_SIZE_WRITERD	0x8000
#define STACK_SIZE_WRITERE	0x8000
#define STACK_SIZE_NOTIFIERF	0x8000

#define STACK_SIZE_TOTAL	( STACK_SIZE_READERA+ \
				STACK_SIZE_READERB + \
				STACK_SIZE_READERC + \
				STACK_SIZE_WRITERD + \
				STACK_SIZE_WRITERE + \
				STACK_SIZE_NOTIFIERF)
```

给进程的结构体添加新的成员

```c
typedef struct s_proc {
	...
	int sleep_ticks;			// remained sleep ticks
	int is_waiting;				// waiting to get Semaphore
}PROCESS;
```

#### proto.h

进程函数声明

```c
/* main.c */
void Reader_A();
void Reader_B();
void Reader_C();
void Writer_D();
void Writer_E();
void Notifier_F();
```

#### global.c

将进程添加到task_table中

```c
PUBLIC	TASK	task_table[NR_TASKS] = {{Reader_A, STACK_SIZE_READERA, "Reader_A"},
					{Reader_B, STACK_SIZE_READERB, "Reader_B"},
					{Reader_C, STACK_SIZE_READERC, "Reader_C"},
                    {Writer_D, STACK_SIZE_WRITERD, "Writer_D"},
                    {Writer_E, STACK_SIZE_WRITERE, "Writer_E"},
                    {Notifier_F, STACK_SIZE_NOTIFIERF, "Notifier_F"},};
```

#### main.c

kernel_main中初始化Process

```c
...
for (i = 0; i < NR_TASKS; i++) {
    ...
    initProcess(p_proc);
    ...
	}
...
void initProcess(PROCESS * p){
	p->is_waiting = FALSE;
	p->sleep_ticks = 0;
}
...

```

定义全局变量统计当前的读写者数量，优先模式和最多读者数量

```c
...
int MAX_READER_NUM;
int reader_count;
int writer_count;
int WRITE_FIRST;
...
```

### 信号量

#### proc.h
声明信号量的结构体

```c
typedef struct s_semaphore
{
	int value;
	// 用数组实现等待队列
	PROCESS* queue[MAX_WAITING_PROCESS_QUEUE_SIZE];
	int queue_start;
	int queue_end;
}SEMAPHORE;
```

#### global.h

声明本次实验需要的所有信号量

```c
EXTERN  SEMAPHORE   reader_count_mutex; // reader_count的修改保护锁
EXTERN  SEMAPHORE   writer_count_mutex; // writer_count的修改保护锁
EXTERN  SEMAPHORE   read_write_mutex;   // 读者写者互斥访问的资源
EXTERN  SEMAPHORE   max_reader_mutex;   // 限制读者个数的保护锁
EXTERN  SEMAPHORE   reader_queue_mutex; // writeFirst的互斥锁，用来给写进程开始后的第一个reader排队
EXTERN  SEMAPHORE   readers_queue_mutex;// writeFirst的互斥锁，用来给写进程开始后第一个reader后的readers排队
EXTERN  SEMAPHORE   writers_queue_mutex;// readFirst的互斥锁，用来给writer排队，后来的reader不用排这个队，即插队，优先级更高
```

#### main.c

声明需要的信号量的全局指针

```c
SEMAPHORE *rc_mutex;
SEMAPHORE *wc_mutex;
SEMAPHORE *rw_mutex;
SEMAPHORE *max_r_mutex;
SEMAPHORE *reader_queue;
SEMAPHORE *readers_queue;
SEMAPHORE *writers_queue; 
```

使用 `init()`  初始化信号量以及清屏以及优先模式以及最多读者数量

```c
PUBLIC int kernel_main()
{
    ...
    init();
    ...
}

void init(){
	WRITE_FIRST = FALSE;
	MAX_READER_NUM = 3;
	clean();
	initSemaphore();
}
void clean(){
	disp_pos = 0;
	for (int i = 0; i < 80*25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
}
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
	// reader_count的访问互斥指针
	rc_mutex->value = 1;
	rc_mutex->queue_start = 0;
	rc_mutex->queue_end = 0;
    // 以下忽略对其他信号量的初始化
	...	
}
```

### PV操作

#### proc.c

```c
PUBLIC void sys_sem_p(SEMAPHORE *s){
	s->value--;
	if (s->value<0)
	{
		p_proc_ready->is_waiting = TRUE;
		s->queue[s->queue_end] = p_proc_ready;
		s->queue_end = (s->queue_end+1)%MAX_WAITING_PROCESS_QUEUE_SIZE;
		// 需要重新选择进程
		schedule();
	}
}
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
```

### 打印状态

三个函数打印进程的三个状态

```c
void processReady(int pid){
	char info[20] = "A Ready|";
	info[0] = info[0]+pid;
	print_color(info,pid+1);
}
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
```

### 修改调度算法

#### clock.c
每次时钟中断的时候，tick增加，正在sleep的进程剩余sleep时间减少

```c
PUBLIC void clock_handler(int irq)
{
	ticks++;
	PROCESS *p;
	for(p = proc_table; p < proc_table+NR_TASKS; p++){
		if (p->sleep_ticks > 0) {
			p->sleep_ticks--;
		}		
	}
	if (k_reenter != 0) {
		return;
	}
	schedule();
}
```

#### proc.c

修改schedule函数

```c
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
```

### 读者

#### 以Reader_A为例
所有的reader调用Reading_Process(int, int)

```c
void Reader_A()
{
	Reading_Process(0,2); // pid, time_slice
}
```

#### Reading_Process(int pid, int time_slice)

```c
void Reading_Process(int pid, int time_slice){
	while (1) {
		processReady(pid);
		P(max_r_mutex);
		if (WRITE_FIRST)
		{
            // 写优先
			// 如果有进程已经申请了写，后续的第一个读请求就会堵塞在reader_queue，其他的读请求就会堵塞在readers_queue
			// readers_queue的作用是让reader_queue最多只有一个reader排队，后来的读请求就可以插队到reader_queue里面
			P(readers_queue);
				P(reader_queue);
					P(rc_mutex);
					reader_count++;
					if (reader_count == 1)
					{
						// 读过程中禁止写, 而其他新就绪读的进程不会被堵塞
						P(rw_mutex);
					}
					V(rc_mutex);
				V(reader_queue);
			V(readers_queue);
		}
		else
		{
            // 读优先，写进程全部在rc_mutex中
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
```

### 写者

#### 以Writer_A为例
所有的writer调用Writing_Process(int, int)

```c
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
```

### 读优先

* 由于原本代码逻辑就是读优先，因为读者是不用等待rw_mutex释放的
* 在写者代码中添加writers_queue，则无论当前进程是读还是写
   - 新来的读进程直接在rw_mutex中排队（当前是写）
   - 而新来的写进程需要先在writers_queue排队，然后才进入rw_mutex排队

### 写优先举例

> 类似于读优先的思路

进程就绪的顺序为E A B C D E

* E开始写 开启了reader_queue 和 rw_mutex
* A来 开启了readers_queue 堵塞在reader_queue中
* B来 堵塞在readers_queue中
* C来 堵塞在readers_queue中
* D来 堵塞在rw_mutex中
* E写完 退出rw_mutex 没有释放reader_queue 因为现在writer_count=2
* D停止在rw_mutex中堵塞 开始写
* 然后D写完 释放了rw_mutex 释放了reader_mutex
* A从reader_mutex出来读 开启了rw_mutex
* E又回来 赶在A释放readers_queue之前，打开reader_mutex
   - 导致B从readers_queue出来 继续堵塞在reader_mutex中
* E堵塞在rw_mutex中 等A读完后 释放rw_mutex
* E开始写

### F

```c
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
```

### 饿死

在读优先和写优先，均会导致非优先进程饿死，可以在优先进程中添加休息代码，不让优先进程一直循环，方便非优先进程在其空余的时候插入

```c
void Writing_Process(int pid, int time_slice){
    ...
    // 一定程度上防止饿死，让进程执行完后休息一下（防累死）
    // milli_delay(3*MS_PER_TIME_SLICE);
}
void Reading_Process(int pid, int time_slice){
    ...
    // 一定程度上防止饿死，让进程执行完后休息一下（防累死）
    // milli_delay(2*MS_PER_TIME_SLICE);
}
```

## 问题清单

1. 进程是什么

系统中资源调度的最小单位，进程是一个具有一定独立功能的程序关于某个数据集合上的一次运行活动，

2. 进程表是什么

进程表是存储进程状态信息的数据结构，是进程存在唯一标识。进程运行过程中如果被打断，关于这个进程的状态就会存入进程表中

3. 进程栈是什么

进程运行时自身的堆栈，

4. 当寄存器的值已经被保存到进程表内，esp应该指向何处来避免破坏进程表的值？

在各个寄存器的值被压入进程表之后，让esp指向内核栈，避免之后的任何栈操作破坏进程表的值

5. tty是什么

terminal，连接console以及keyboard，文本的输入输出环境

6. 不同的tty为什么输出不同的画面在同一个显示器上

每个tty连接有一个console，每个console的起始地址对应着显存上的不同地址，同一个显示器对应的显存地址不同，输出画面也就不同。

7. 解释tty任务执行过程

循环遍历每个tty，判断是否是当前tty，并且对应的console需要时当前控制台的，就读取键盘缓冲区进入tty缓冲区，并且如果tty缓冲区有内容，则显示其中的内容

8. tty结构体中大致包含哪些内容
   - tty输入缓冲区
   - 两个指针，一个指向缓冲区的head，用于放入后续进入的字符，一个指向缓冲区的tail，用于打印字符
   - 一个计数器count，用于统计tty缓冲区的字符数量
9. console结构体中大致包含哪些内容
   - 现在显示的内容在显存对应的地址
   - console对应的显存中的起始地址
   - 当前控制台占据的显存的大小
   - 当前光标的地址
