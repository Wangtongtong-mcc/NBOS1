; ---------------------------------------------------------
; 进入保护模式后，BIOS的中断处理程序如int 10已不能再使用
; print主要功能用于向屏幕上输出字符串
; 输入：要打印的字符串的内存地址（栈中）
; 输出：无
; ---------------------------------------------------------

[bits 32]
global print
global put_char
global clear_screen
global printn
%define VIDEO 0xc00b8000

print:
; 备份寄存器环境
    pushad                  ; 备份所有通用寄存器
; 获取要打印的参数地址
    mov ebp, esp            ; 使用ebp获取栈中的参数，以防止过程中有栈操作导致esp的改变
    mov ebx, [ebp+4*9]      ; 栈中需跳过刚备份的8个通用寄存器及返回地址，才是参数的地址
    xor ecx, ecx
; 清除当前屏幕
    call clear_screen
start_print:
; 调用put_char打印字符，字符串通常以'\0'结尾, ascii码位0
    mov byte cl, [ebx]      ; 要打印的字符串中的字符
    cmp cl, 0               ; 检测是否已到字符串末尾
    je end_print
    push ecx
    call put_char           ; 调用put_char打印当前字符
    inc ebx
    jmp start_print
; 已检测到字符串末尾，打印完成，恢复环境并返回
end_print:
    popad
    ret

; ---------------------------------------------------------
; put_char主要功能用于向屏幕上输出单个字符
; 输入：栈中的字符
; 输出：无
; ---------------------------------------------------------
put_char:
; 备份当前环境
    pushad
; 获取当前光标位置
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Miscellaneous Output Register中的I/O AS位用于选择CRT
;   Controller寄存器组的地址，默认值为1
; CRT Controller Register的索引端口为0x3d4，数据读写端口为0x3d5
; CRT Controller寄存器组中索引为0x0e和0x0f的寄存器存储光标位置的
;   高8位和低8位
; ***对于in和out指令，操作数中使用的通用寄存器只能是AL或AX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    xor eax, eax
    xor ebx, ebx

    mov al, 0x0e
    mov dx, 0x03d4
    out dx, al              ; 将光标位置高八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    in al, dx               ; 将光标位置高八位从数据寄存器中复制到AL
    mov ah, al              ; 将光标位置高八位存入到AH中

    mov al, 0x0f
    mov dx, 0x03d4
    out dx, al              ; 将光标位置低八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    in al, dx               ; 将光标位置低八位从数据寄存器中复制到AL
    mov bx, ax              ; 现在BX中为光标的当前位置

; 根据栈中的字符，确认处理流程
    mov ebp, esp
    mov cl, [ebp+4*9]  		; 栈中需跳过刚备份的8个通用寄存器及返回地址，才是参数字符
    cmp cl, 0x08			; 0x08为退格控制字符
    je handle_backspace
    cmp cl, 0x0a			; 0x0a为换行控制字符
    je handle_line_feed
    cmp cl, 0x0d			; 0x0d为回车控制字符
    je handle_carriage_return
    jmp handle_other		; 其他普通字符

; ---------------------------------------------------------
; handle_backspace用于处理退格符号，删除上一个字符，光标位置退1
handle_backspace:
    dec bx                                  ; 光标位置回到最后一个字符的位置
    shl bx, 1                               ; 光标位置乘2即字符在显存中的位置，低字节为字符ascii码，高字节为显示属性
    mov word [VIDEO+ebx], 0x0700       		; 将最后一个字符设置为空字符
    shr bx, 1
    jmp set_cursor                          ; 设置CRT Controller寄存器组中索引为0x0e和0x0f的寄存器中的光标位置

; ---------------------------------------------------------
; handlehandle_line_feed和handle_carriage_return处理换行和回车
handle_line_feed:
handle_carriage_return:

    xor dx, dx
    mov word si, 80
    div si                                  ; 此时dx中是当前光标位置除以80的余数
                                            ; 即是当前行的字符数
    sub bx, dx                              ; bx中为当前行行首的位置
    cmp bx, 1920                            ; 判断是否已经是屏幕的最后一行
    je scroll_screen
    add bx, 80                              ; 此时bx为下一行行首，完成换行
    jmp set_cursor

; ---------------------------------------------------------
; handle_other处理其他可打印字符
handle_other:
    shl bx, 1                               ; 光标位置乘2为显存中下一个字符的位置
    mov [ebx+VIDEO], cl
    inc bx
    mov byte [ebx+VIDEO], 0x07         		; 将待显示字符和属性写入显存
    shr bx, 1
    inc bx                                  ; 推进光标位置
    cmp bx, 2000
    jg scroll_screen                        ; 若光标已到屏幕末尾，则需滚屏，否则仅设置光标位置即可
    jmp set_cursor

; ---------------------------------------------------------
; scroll_screen用于在光标位置达到2000时，滚动屏幕
; 具体做法就是将1-24行的内容移到0-23行，24行全部置空，光标位于24行行首
scroll_screen:
    push ds
    mov ax, 0x0020
    mov ds, ax                              ; movsw要用到ds

    mov ecx, 1920
    mov esi, VIDEO
    mov edi, VIDEO
    add edi, 0xa0
    cld
    rep movsw                               ; 将1-24行的内容移到0-23行,共80*24个字

    mov bx, 1920
    shl bx, 1
    mov ecx, 80
set_0:
    mov byte [ebx+VIDEO], 0x00
    inc bx
    mov byte [ebx+VIDEO], 0x07
    inc bx
    loop set_0                              ; 将最后一行设为空

    mov bx, 1920                            ; 光标位置为24行行首

    pop ds
    jmp set_cursor

; ---------------------------------------------------------
; set_cursor用于设置显存寄存器中的光标位置
; CRT Controller寄存器组中索引为0x0e和0x0f的寄存器中的光标位置
set_cursor:
    mov al, 0x0e
    mov dx, 0x03d4
    out dx, al              ; 将光标位置高八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    mov al, bh
    out dx, al              ; 将光标位置高八位写入0x0e数据寄存器

    mov al, 0x0f
    mov dx, 0x03d4
    out dx, al              ; 将光标位置低八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    mov al, bl
    out dx, al              ; 将光标位置低八位写入0x0e数据寄存器

; put_char处理完成，恢复环境并返回
    popad
    ret 4


; ---------------------------------------------------------
; clear_screen主要功能用于清除屏幕上的内容
; 具体做法：将屏幕全部置为黑底白字的空字符，光标位置置为0
; ---------------------------------------------------------
clear_screen:
    push ecx
    push ebx
    mov ecx, 2000
    mov ebx, VIDEO
clear:
    mov word [ebx], 0x0700
    add ebx, 2
    loop clear

    mov ebx, 0

    mov al, 0x0e
    mov dx, 0x03d4
    out dx, al              ; 将光标位置高八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    mov al, bh
    out dx, al              ; 将光标位置高八位写入0x0e数据寄存器

    mov al, 0x0f
    mov dx, 0x03d4
    out dx, al              ; 将光标位置低八位寄存器的索引号写入索引寄存器
    mov dx, 0x03d5
    mov al, bl
    out dx, al              ; 将光标位置低八位写入0x0e数据寄存器

    pop ebx
    pop ecx
    ret



; ---------------------------------------------------------
; printn主要功能用于向屏幕输出数字
; 输入：栈中待打印数字
; ---------------------------------------------------------
printn:
	pushad
	mov ebp, esp
	mov eax, [ebp+4*9]				; 跳过刚刚保存的8个通用寄存器和返回地址，取得待打印数字
	mov edx, eax					; 备份一下









