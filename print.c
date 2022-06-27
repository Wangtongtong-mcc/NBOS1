#include "kernel.h"
#include "print.h"
#include "memory.h"
#include "x86.h"

unsigned int get_cursor(){

	outb(VEDIO_INDEX_PORT,HIGH_CURSOR_INDEX);
	unsigned char high_cursor;
	inb(VEDIO_DATD_PORT,high_cursor);

	outb(VEDIO_INDEX_PORT,LOW_CURSOR_INDEX);
	unsigned char low_cursor;
	inb(VEDIO_DATD_PORT,low_cursor);

	return ((unsigned int)high_cursor << 8) + low_cursor;
}

void set_cursor(unsigned int cursor){
	unsigned char high_cursor = (cursor & 0x0000ff00) >> 8;
	unsigned char low_cursor = cursor & 0x000000ff;

	outb(VEDIO_INDEX_PORT,HIGH_CURSOR_INDEX);
	outb(VEDIO_DATD_PORT,high_cursor);

	outb(VEDIO_INDEX_PORT,LOW_CURSOR_INDEX);
	outb(VEDIO_DATD_PORT, low_cursor);
}

void scroll_screen(){
	unsigned char * source = (unsigned char * )(VEDIO_POSITION + 80*2);
	unsigned char * dest = (unsigned char *) VEDIO_POSITION;
	unsigned int size = 24*80*2;
	memmov(source,dest,size);
	unsigned int cursor = get_cursor();
	set_cursor(cursor - 80);
}



void update_cursor(unsigned int current_cursor){
	// 如果是屏幕最后一个字符，需要滚动屏幕，否则直接更新光标寄存器中的值
	if (current_cursor == LAST_CURSOR){
		scroll_screen();
		set_cursor(LAST_CURSOR-80);
	} else{
		set_cursor(current_cursor + 1);
	}

}

void set_char(unsigned int current_cursor,unsigned char a){
	unsigned char * p = (unsigned char *)(current_cursor*2 + VEDIO_POSITION);
	*p = a;
	*(p+1) = CON_ATTR;
}

void put_char0(unsigned char a, unsigned int current_cursor){
	set_char(current_cursor, a);
	update_cursor(current_cursor);
}



void backspace(unsigned int current_cursor){
	if (current_cursor == 0){
		return;
	}
	set_cursor(current_cursor - 1);
	set_char(current_cursor - 1,0);
}

void linefeed(unsigned int current_cursor){
	if ((current_cursor/80) == 24 ){
		scroll_screen();
		set_cursor(LAST_CURSOR- 80);
	}
	unsigned int cursor = (current_cursor/80 + 1)*80;
	set_cursor(cursor);

}

void put_char(unsigned char a){
	unsigned int current_cursor = get_cursor();
	switch (a){
		case BACKSPACE:
			backspace(current_cursor);
			break;
		case LINEFEED:
		case CARRIAGE_RETURN:
			linefeed(current_cursor);
			break;
		default:
			put_char0(a,current_cursor);
	}
}


void print(unsigned char * p){
	while (*p != '\0'){
		put_char(*p);
		p++;
	}
}

char * num2ascii(unsigned int number, char * asciipos){
	for(int i = 0; i < 8; i++){
		char a = (number >> (i*4)) & 0x0000000f;
		asciipos[7-i] = a + (a < 10 ? 48 : 55);
	}
	asciipos[8] = 0;
}

void printn(unsigned int number){
	char asciipos[9];
	num2ascii(number,asciipos);
	print(asciipos);
}

void clear_screen(){
	// 将屏幕都设置为黑底白字的空字符，光标位置置为0
	for(int i = 0; i < 2000; i++){
		set_char(i,0);
	}
	set_cursor(0);
}