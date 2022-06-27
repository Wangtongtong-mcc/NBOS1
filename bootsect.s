; ---------------------------------------------------------
; bootsect.s为主引导扇区代码，由BIOS加载到内存0x7c00处.
; 主要功能用于加载loader到内存中，开启保护模式，并将控制转移到loader中.
; ---------------------------------------------------------
[bits 16]
boot:
    mov ax, 0x07c0
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00

; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   使用BIOS提供的中断清理BIOS代码在屏幕的输出
;   INT 10,6 - Scroll Window Up
;       AH = 06
;    	AL = number of lines to scroll, previous lines are
;        	     blanked, if 0 or AL > screen size, window is blanked
;    	BH = attribute to be used on blank line
;    	CH = row of upper left corner of scroll window
;    	CL = column of upper left corner of scroll window
;    	DH = row of lower right corner of scroll window
;   	DL = column of lower right corner of scroll window
;    	returns nothing
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov ah,0x06
    mov al,0
    mov ch,0
    mov cl,0
    mov dh,24
    mov dl,79
    mov bh,0x07 ;黑底白字
    int 0x10

; 加载loader前，告诉用户系统即将启动
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   使用BIOS提供的中断重新设置光标的位置
;   INT 10,2 - Set Cursor Position
;       AH = 02
;    	BH = page number (0 for graphics modes)
;    	DH = row
;    	DL = column
;       returns nothing
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    mov ah,0x02
    mov bh,0x00
    mov dx,0x0000

; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   使用BIOS提供的中断向屏幕输出
;   INT 10,13-Write String (BIOS versions from 1/10/86)
;       AH = 13h
;    	AL = write mode (see bit settings below)
;    	   = 0 string is chars only, attribute in BL, cursor not moved
;    	   = 1 string is chard only, attribute in BL, cursor moved
;    	   = 2 string contains chars and attributes, cursor not moved
;    	   = 3 string contains chars and attributes, cursor moved
;    	BH = video page number
;    	BL = attribute if mode 0 or 1 (AL bit 1=0)
;    	CX = length of string (ignoring attributes)
;    	DH = row coordinate
;   	DL = column coordinate
;    	ES:BP = pointer to string
;       returns nothing
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 加载es，便于int指令的执行
    mov ax, 0x07c0
    mov es, ax

    mov ax, 0x1301
    mov bx, 0x0007
    mov cx, 21
    mov bp, msg1
    int 0x10

; 此部分代码用于将loader加载到内存中
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   假设：loader代码位于第2个扇区
;   总长度为4个扇区，共2kB
;   loader加载到内存0x7e00处
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   使用BIOS提供的中断读取磁盘
;   INT 13,2 - Read Disk Sectors
;       AH = 02
;       AL = number of sectors to read	(1-128 dec.)
;       CH = track/cylinder number  (0-1023 dec., see below)
;       CL = sector number  (1-17 dec.)
;       DH = head number  (0-15 dec.)
;       DL = drive number (0=A:, 1=2nd floppy, 80h=drive 0, 81h=drive 1)
;       ES:BX = pointer to buffer
;       on return:
;        AH = status  (see INT 13,STATUS)
;        AL = number of sectors read
;        CF = 0 if successful
;           = 1 if error
; ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 加载es，便于int指令的执行
    mov ax, 0x07e0
    mov es, ax

load_setup:
    mov ax, 0x0204
    mov bx, 0x0000
    mov dx, 0x0080
    mov cx, 0x0002
    int 0x13
    jnc pro_mode
    mov	dx, 0x0000
    mov	ax, 0x0000
    int	0x13
    jmp load_setup

pro_mode:
; 首先禁用中断
	cli
; 打开A20
	in al, 0x92
	or al, 0x02
	out 0x92, al
; 加载GDTR
	lgdt [gdtr]
; 设置cr0，进入保护模式
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

; 将控制转移到loader代码中
    jmp dword 0x0008:0x7e00


; 引导扇区数据区域
;---------------------------------------------------------
gdt:
	dd 0x00000000					; gdt 0号槽为空描述符
    dd 0x00000000

    dd 0x0000ffff					; 1号槽为代码段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可读可执行的代码段)
    dd 0x00cf9a00

    dd 0x0000ffff					; 2号槽为数据段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可写的数据段)
    dd 0x00cf9200
gdtr:
	dw 23
    dd gdt+0x7c00


	msg1 db 'System is starting...'

; 一个有效的主引导扇区必须以0x55, 0xaa两个字节结尾
    times 510-($-$$) db 0
    db 0x55, 0xaa
