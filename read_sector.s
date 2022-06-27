; ---------------------------------------------------------
; 进入保护模式后，BIOS的中断处理程序如int 13已不能再使用
; read_sector.s主要功能用于读取硬盘中的一个扇区
; 输入：起始扇区，内存位置
; 输出：无
; ---------------------------------------------------------
global read_sector
read_sector:
; 备份寄存器环境
    pushad                  ; 备份所有通用寄存器

    mov ebp, esp 			; 用ebp取栈中参数

; 先选择通道（Primary），并向该通道sector count（0x1f2）寄存器写入待读扇区数
    mov dx, 0x1f2
    mov al, 1
    out dx, al

; 向该通道的三个LBA寄存器写入起始扇区号的低24位
	xor eax, eax
	mov eax, [ebp+10*4]

    mov dx, 0x1f3
    out dx, al

    mov al, ah
    mov dx, 0x1f4
    out dx, al

    shr eax, 16
    mov dx, 0x1f5
    out dx, al

; 向device寄存器中写入扇区号的24～27位，并设置第六位开启LBA模式，设置第四位，选择主盘或从盘
    mov al, ah
    mov dx, 0x1f6
    and al, 0x0f
    or al, 0xe0
    out dx, al

; 向command中写入命令（0x20为读， 0x30为写）
    mov al, 0x20
    mov dx, 0x1f7
    out dx, al

; 读取status，判断硬盘是否完成数据准备
check_status:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz check_status

; 若硬盘已准备好，就开始读取数据
; data寄存器为16位，一次读一个字

read_data:
	xor ecx, ecx
    mov cx, 256

    mov dx, 0x1f0
    mov ebx, [ebp+9*4]

start_read:
    in ax, dx
    mov word [ebx], ax
    add ebx, 2
    loop start_read

; 恢复环境，并返回
    popad
    ret



