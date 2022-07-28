#include "x86.h"
#include "print.h"
#include "memory.h"
#include "kernel.h"

#define BUFFERSIZE 512
char keyBuffer[BUFFERSIZE] = {0};
int offset = 0;

// 定义键盘扫描码与ASCII码的映射关系
// 索引为通码，主要映射主键盘（0x1-0x3a）
// keymap为键本身的映射关系，keymaps为键与shift键结合的映射关系
char keymap[] = {
		[1]='\033', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',            // 01为esc,通码02-0b为数字1-9-0, 0c-0e为-，=，退格键
		'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',                  // 0f为tab,10-19为键盘第一排字母，1a-1b为[]，1c为回车
		0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',                          // 1d为左边ctrl，1e-26为第二排字母，27为；，28为',29为`
		0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,                            // 2a为左边的shift,2b为\,2c-32为第三排字母，33-35为,./，
																								 // 36为右边的shift
		'*', 0, ' ', 0};                                                                         // 37为*,38为左alt,39为空格，3a为caps_lock


char keymaps[] = {
		[1]='\033', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
		'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
		0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
		0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
		'*', 0, ' ', 0};

// 控制字符的通断码
#define caps_lock_make 0x3a
#define l_shift_make 0x2a
#define r_shift_make 0x36
#define l_ctrl_make 0x1d
#define l_alt_make 0x38

// 定义控制字符的状态
char status_shift, status_ctrl, status_alt, status_caps_lock;

// 判断扫描码是否为断码
int isBreakCode(char scancode){
	return scancode & 0x80;
}

// 获取当前键盘输入的键的ascii码
char getAscii(void){

	// 从键盘控制器获取扫描码
	char scancode;
	inb(OUTPUT_BUFFER,scancode);


	// 断码不处理，直接返回
	if(isBreakCode(scancode)){
		// 处理shift
		if((scancode == l_shift_make + 0x80) || (scancode == r_shift_make + 0x80)){
			status_shift == 0;
		}
		return 0;
	}

	// 处理通码
	// 处理shift
	if(scancode == l_shift_make || scancode == r_shift_make){
		status_shift = 1;
		return 0;
	}
	if (status_shift == 1){
		return keymaps[scancode];
	}
	return keymap[scancode];

}


// 处理ascii码，将其回显到屏幕上，并存入键盘输入缓存区
void handleCode(char code){
	// 判断参数有效性
	if (code == 0){
		return;
	}

	// 显示到屏幕上
	put_char(code);

	// 将按键写入缓存区
	if(code == '\b' && offset != 0){
		offset--;
		return;
	}
	if(offset >= BUFFERSIZE){
		return;
	}
	keyBuffer[offset++] = code;
}

// 键盘中断处理函数
void do_keyboard_handler(void){

	// 获取ascii码
	char code = getAscii();

	// 处理按键
	handleCode(code);
}


// 读取缓存区的标准输入，为系统调用sys_read函数fd=STDIN_FILENO时的处理函数
int readStdin(char * buf){

	// 寻找回车键的index
	int index = -1;
	for (int i = 0; i < BUFFERSIZE; i++){
		if(keyBuffer[i] == '\n'){
			index = i;
			break;
		}
	}

	if(index == -1){
		return -1;
	}
	// 将回车键前的字符串写入buf
	memcopy(keyBuffer,buf,index+1);

	// 更新缓存区
	memcopy(keyBuffer+index+1,keyBuffer,BUFFERSIZE-index-1);
	offset -= (index -1);

	return index+1;





}