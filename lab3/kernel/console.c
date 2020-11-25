
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h" // 存储需要的常量的值
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);
PRIVATE void search(CONSOLE* p_con);
PRIVATE void make_red(u8* start, u8* end);
PRIVATE void clear_red(CONSOLE *p_con);
PRIVATE void push(CONSOLE* p_con,int op, char key);
PRIVATE OPERATION* pop(CONSOLE* p_con);
PRIVATE OPERATION* get_empty_space_in_op_table();

/*======================================================================*
			   init_screen
			   会给每个tty分配自己的console
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	// V_MEM_SIZE = 0x8000 32KB 除以2是可以容纳的字符数量
	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	// 定义了三个console,每个console的size就是总的显存大小整除以3
	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	// 默认上一个光标的位置也在最开始处
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	// 默认初始的时候是输入状态
	p_tty->p_console->mode = 0;
	p_tty->p_console->current_char_color = DEFAULT_CHAR_COLOR;
	p_tty->p_console->input_start_addr = p_tty->p_console->original_addr;

	// 初始化操作的链表head
	OPERATION  *o = get_empty_space_in_op_table();
	// 头节点唯一标识符 op = 2 key = '\0'
	o->operation = 2;
	o->key = '\0';
	p_tty->p_console->op_stack_head= o;
	p_tty->p_console->ops_num = 0;

	// 由于第一个控制台打印了loader执行时的信息，所以初始的时候是有值的
	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}

	// else {
	// 	out_char(p_tty->p_console, nr_tty + '0');
	// 	out_char(p_tty->p_console, '#');
	// }
	clean_console(p_tty->p_console);
	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	switch(ch) {	
	case '\n':
		if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
				p_con->p_cursor = p_con->cursor;
				p_con->cursor = p_con->original_addr + SCREEN_WIDTH * 
					((p_con->cursor - p_con->original_addr) /
					SCREEN_WIDTH + 1);
				// 把回车当作'\0'填入当前cursor中，然后新的cursor与当前cursor之前填满'\n'
				*p_vmem++ = '\0';
				*p_vmem++ = p_con->current_char_color;
				for (int i = p_con->p_cursor+1; i < p_con->cursor; i++)
				{
					*p_vmem++ = '\n';
					*p_vmem++ = BLACK;
				}
				push(p_con,0,'\n');
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->input_start_addr) {
			p_con->p_cursor = p_con->cursor;
			if (*(p_vmem-2) == '\n')
			{
				int count = 1;
				while (*(p_vmem-2) != '\0')
				{
					count++;
					*(--p_vmem) = DEFAULT_CHAR_COLOR;
					*(--p_vmem) = ' ';
				}
				p_con->cursor -= count;
				push(p_con,1,'\n');
			}
			else if (*(p_vmem-2) == '\t')
			{
				for (int i = 0; i < 4; i++)
				{
					*(--p_vmem) = DEFAULT_CHAR_COLOR;
					*(--p_vmem) = ' ';
				}
				p_con->cursor -=4;
				push(p_con,1,'\t');
			}
			else
			{
				push(p_con,1,*(p_vmem-2));
				*(--p_vmem) = DEFAULT_CHAR_COLOR;
				*(--p_vmem) = ' ';
				p_con->cursor -=1;
			}
		}
		break;
	case '\t':
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 4) {
			for (int i = 0; i < 4; i++)
			{
				 *p_vmem++ = '\t';
				 *p_vmem++ = BLACK;
			}
			p_con->p_cursor = p_con->cursor;
			p_con->cursor+=4;
			push(p_con,0,'\t');
		}
		break;
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			*p_vmem++ = ch;
			*p_vmem++ = p_con->current_char_color;
			p_con->p_cursor = p_con->cursor;
			p_con->cursor++;
			push(p_con,0,ch);
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}
	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
        set_cursor(p_con->cursor);
        set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	// 这是用来设置光标位置的寄存器,高位写索引值，地位写value
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			// 整个屏幕往上移动，光标位置变小
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			// 整个屏幕往下移动，光标位置变大
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

/*======================================================================*
			control z操作
 *======================================================================*/
PUBLIC void undo(CONSOLE* p_con){	
	OPERATION *o = pop(p_con);
	// 之前是增加一个字符
	if (o->operation == 0)
	{
		out_char(p_con,'\b');
		// 这个操作不能进入栈中
		pop(p_con)->operation = -1;
	}
	// 之前是删去一个字符
	else if (o->operation == 1)
	{
		out_char(p_con,o->key);
		// 这个操作不能进入栈中
		pop(p_con)->operation = -1;
	}
	else
	{

	}
	o->operation = -1;
}

/*======================================================================*
			clean_console
 *======================================================================*/

PUBLIC void clean_console(CONSOLE* p_con) {
	// console的左上角
	u8* start_vmem = (u8*)(V_MEM_BASE+p_con->original_addr*2);
	// console的当前cursor所对应的显存的位置
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	for(;start_vmem < p_vmem; start_vmem += 2) {
		*start_vmem = ' ';
		*(start_vmem+1) = DEFAULT_CHAR_COLOR;
	}
	p_con->cursor = p_con->original_addr;
	flush(p_con);
}

/*======================================================================*
			switch_mode
 *======================================================================*/
PUBLIC void switch_mode(CONSOLE* p_con){
	if (p_con->mode == 0)
	{
		p_con->mode = 1;
		p_con->current_char_color = RED_CHAR_COLOR;
		p_con->input_start_addr = p_con->cursor;
	}
	else if (p_con->mode == 1)
	{
		p_con->mode = 2;
		search(p_con);
	}
	else
	{
		p_con->mode = 0;
		p_con->current_char_color = DEFAULT_CHAR_COLOR;
		clear_red(p_con);
		p_con->cursor = p_con->input_start_addr;
		p_con->input_start_addr = p_con->original_addr;

	}
	flush(p_con);
}

/*======================================================================*
			search
 *======================================================================*/
PRIVATE void search(CONSOLE* p_con){
	// 如果要搜寻的字符串的长度大于存在的文本长度，那么就肯定并不存在
	if ((p_con->cursor-p_con->input_start_addr)>(p_con->input_start_addr-p_con->original_addr))
	{
		return;
	}
	u8 *str_start_vem = (u8*)(V_MEM_BASE+p_con->input_start_addr*2);
	u8 *str_end_vem = (u8*)(V_MEM_BASE+p_con->cursor*2);
	u8 *text_start_vem = (u8*)(V_MEM_BASE+p_con->original_addr*2);
	u8 *text_end_vem = (u8*)(V_MEM_BASE+p_con->input_start_addr*2);
	
	u8 *k;
	for (k = text_start_vem; k < text_end_vem; k+=2)
	{
		if (*k == *str_start_vem)
		{
			u8 *i = k;
			u8 *j = str_start_vem;
			while (j<str_end_vem)
			{
				if (*i != *j)
				{
					break;
				}
				i+=2;
				j+=2;	
			}
			if (j == str_end_vem)
			{
				make_red(k,i);
			}
		}
	}	
}

/*======================================================================*
			make_red
 *======================================================================*/
PRIVATE void make_red(u8* start, u8* end){
	for (u8* i = start; i < end; i+=2)
	{
		if (*i != '\t'&& *i != '\n')
		{
			*(i+1) = RED_CHAR_COLOR;
		}
	}
}

/*======================================================================*
			clear_red
 *======================================================================*/
PRIVATE void clear_red(CONSOLE *p_con){
	u8 *str_start_vem = (u8*)(V_MEM_BASE+p_con->input_start_addr*2);
	u8 *str_end_vem = (u8*)(V_MEM_BASE+p_con->cursor*2);
	u8 *text_start_vem = (u8*)(V_MEM_BASE+p_con->original_addr*2);
	u8 *text_end_vem = (u8*)(V_MEM_BASE+p_con->input_start_addr*2);
	u8 *k;
	for (k = text_start_vem; k < text_end_vem; k+=2){
		if (*(k+1) == RED_CHAR_COLOR)
		{
			*(k+1) = DEFAULT_CHAR_COLOR;
		}
	}
	for (k = str_start_vem; k < str_end_vem; k+=2){
		*(k) = ' ';
		*(k+1) = DEFAULT_CHAR_COLOR;
	}
}

/*======================================================================*
			push
 *======================================================================*/
PRIVATE void push(CONSOLE* p_con,int op, char key){
	OPERATION *o = get_empty_space_in_op_table();
	o->key = key;
	o->operation = op;
	o->next = p_con->op_stack_head->next;
	p_con->op_stack_head->next = o;
	p_con->ops_num++;
}

/*======================================================================*
			pop
			将head之后的节点pop出来
 *======================================================================*/
PRIVATE OPERATION* pop(CONSOLE* p_con){
	if (p_con->ops_num == 0)
	{
		return p_con->op_stack_head;
	} 
	OPERATION *o = p_con->op_stack_head->next;
	p_con->op_stack_head->next = o->next;
	p_con->ops_num--;
	return o;
}

/*======================================================================*
			get_empty_space_in_op_table
			从ops_table中找到可用的空间，然后取他的指针返回，假设肯定存在一个空间，因为空间足够大
 *======================================================================*/
PRIVATE OPERATION* get_empty_space_in_op_table(){
	for (int i = 0; i < 80*25; i++)
	{
		if (operation_table[i].operation == -1)
		{
			return operation_table+i;
		}
	}
}