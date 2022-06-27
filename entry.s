; entry.s为内核一部分
; 用于开始内核主要内容前进行分页

[bits 32]
extern pagedir
extern pagetable
extern main

%define VIR_2_PHY(x) x-0xc0000000

global entry
global gdt
entry:
	lgdt [VIR_2_PHY(gdtr)]										; 重新加载gdtr

	mov eax, VIR_2_PHY(pagetable)								; eax中为页表的物理地址
	mov ebx, VIR_2_PHY(pagedir)									; ebx中的为页目录的物理地址

; 填充页目录（0xc0000000-0xc00fffff映射到物理内存0x000000000-0x000fffff）
	or eax, 0x00000007											; 属性：RW和P位为1,US为1,表示用户属性,所有特权级别都可以访问
	mov [ebx], eax
	mov [ebx+0xc00], eax										; 分页后还在内核中运行，应保证物理地址与虚拟地址相同
	mov edx, ebx
	or edx, 0x00000007
	mov [ebx+4092], edx											; 最后一个pde可以找到页目录本身这个页

; 创建内核其他页表的pde，页表从1MB开始
; 内核物理空间安排为物理内存的一半，即256MB，每个页表管理4MB内存，一共需要64个页表
	mov eax, VIR_2_PHY(pagedir)
	mov edx, 0x00100000
	or edx, 0x00000007
	mov ecx, 64
	mov esi, 769
fill_pagedir:
	mov [eax+esi*4], edx
	add edx, 0x1000
	inc esi
	loop fill_pagedir

; 填充页表（0xc0000000-0xc00fffff映射到物理内存0x000000000-0x000fffff）再加64个页表
	mov eax, VIR_2_PHY(pagetable)								; eax中为页表的物理地址
	mov ecx, 320
	mov edx, 0x00000000
	or edx, 0x00000007											; 属性：RW和P位为1,US为1,表示用户属性,所有特权级别都可以访问
	mov esi, 0
fill_pagetable:
	mov [eax+esi*4], edx
	add edx, 0x1000
	inc esi
	loop fill_pagetable


; 加载PDBR
	or ebx, 0x00000000											; 属性：PCD和PWT为0
	mov cr3, ebx

; 开启分页
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

; 设置栈指针
	mov esp, 0xc0010000

; 跳到main函数
	mov eax, main
	jmp eax



;---------------------------------------------------------
gdt:
	dd 0x00000000					; gdt 0号槽为空描述符
    dd 0x00000000

    dd 0x0000ffff					; 1号槽为内核代码段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可读可执行的代码段)
    dd 0x00cf9a00

    dd 0x0000ffff					; 2号槽为内核数据段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可写的数据段)
    dd 0x00cf9200

	dd 0x0000ffff					; 3号槽为用户代码段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可读可执行的代码段)
	dd 0x00cffa00

	dd 0x0000ffff					; 4号槽为用户数据段描述符(BASE:0x00000000, LIMIT:0xfffff, 粒度：4kB, 可写的数据段)
	dd 0x00cff200

	times 10 dd 0					; 预留5个段描述符的位置

gdtr:
	dw ($-gdt-1)
    dd gdt






