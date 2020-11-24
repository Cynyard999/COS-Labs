
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

/* OPERATION */
typedef struct s_operation
{
	unsigned int operation; /* 0: input 1:delete */
	char key;
	struct s_operation * next;
}OPERATION;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			/* 当前光标位置 */
	unsigned int	p_cursor;			/* 之前光标位置 */
	unsigned int	ops_num ; 			/* op栈的元素数量 */
	unsigned int	input_start_addr; 	/* 开始输出的位置，默认是original_addr,在搜索的时候会改变，防止搜索的时候删掉非搜索字符	  */
	unsigned int 	mode; 			/* 当前的状态，0: input_mode 1: search_input_mode  2: search_output_mode*/
	u8				current_char_color;	/* 表示当前打印的字符的颜色 */
	struct s_operation * op_stack_head;	/* 表示当前console对应的操作栈的栈头指针 */
}CONSOLE;


#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */
#define RED_CHAR_COLOR		0x04	/* 0000 0100 黑底红字 */


#endif /* _ORANGES_CONSOLE_H_ */
