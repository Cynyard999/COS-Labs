
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "proc.h"

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void Reader_A();
void Reader_B();
void Reader_C();
void Writer_D();
void Writer_E();
void Notifier_F();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
PUBLIC void     sys_milli_sleep(int mills);
PUBLIC void     sys_print(char * str);
PUBLIC  void    sys_print_color(char * str, int color);
PUBLIC void     sys_sem_p(SEMAPHORE *s);
PUBLIC void     sys_sem_v(SEMAPHORE *s);


/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();
PUBLIC  void    milli_sleep(int mills);
PUBLIC  void    print(char * str);
PUBLIC  void    print_color(char * str, int color);
PUBLIC  void    P(SEMAPHORE *s);
PUBLIC  void    V(SEMAPHORE *s);
