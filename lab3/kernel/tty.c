
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, u32 key);

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	init_keyboard();

	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
		init_tty(p_tty);
	}
	select_console(0);
	
	// 初始化operation_table
	for (int i = 0; i < 80*25 ; i++)
	{
		operation_table[i].operation = -1;
		operation_table[i].key = '\0';
	}
	int t = get_ticks();
	while (1) {
		// 3个terminal 每个都read和write
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
		// 本来有HZ的，但是作者define的HZ是100，有问题，应该是1000？
		// ((get_ticks() - t) * 1000 / HZ) < milli_sec
		if (((get_ticks() - t) *1000 / 1000)>20*1000){
			for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) {
				// 只有输入状态的时候才刷新
				if (p_tty->p_console->mode == 0)
				{
					clean_console(p_tty->p_console);
				}
			}
			t = get_ticks();
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	init_screen(p_tty);
}

/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(TTY* p_tty, u32 key)
{
	char output[2] = {'\0', '\0'};
	if (p_tty->p_console->mode == 2 && (key & MASK_RAW) != ESC){	
		return;
	}
	if (!(key & FLAG_EXT)) {
		put_key(p_tty, key);
	}
	else {
			int raw_code = key & MASK_RAW;
			switch(raw_code) {
				case UNDO:
					undo(p_tty->p_console);
					break;
				case ESC:
					if (p_tty->p_console->mode != 1)
					{
						switch_mode(p_tty->p_console);
						break;
					}
					break;
				case ENTER:
					if (p_tty->p_console->mode == 1)
					{
						switch_mode(p_tty->p_console);
						break;
					}
					put_key(p_tty, '\n');
					break;
				case BACKSPACE:
					put_key(p_tty, '\b');
					break;
				case TAB:
					put_key(p_tty, '\t');
					break;
				case UP:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_DN);
					}
					break;
				case DOWN:
					if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R)) {
						scroll_screen(p_tty->p_console, SCR_UP);
					}
					break;
				case F1:
				case F2:
				case F3:
				case F4:
				case F5:
				case F6:
				case F7:
				case F8:
				case F9:
				case F10:
				case F11:
				case F12:
					/* Alt + F1~F12 */
					if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R)) {
						select_console(raw_code - F1);
					}
					break;
				default:
					break;
			}	
	}	
}

/*======================================================================*
			      put_key
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) {
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
			      tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	if (is_current_console(p_tty->p_console)) {
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	if (p_tty->inbuf_count) {
		char ch = *(p_tty->p_inbuf_tail);
		p_tty->p_inbuf_tail++;
		if (p_tty->p_inbuf_tail == p_tty->in_buf + TTY_IN_BYTES) {
			p_tty->p_inbuf_tail = p_tty->in_buf;
		}
		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


