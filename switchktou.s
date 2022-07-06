; ---------------------------------------------------------
; switchktou主要用于将内核上下文切换为用户进程上下文
; 参数分别为内核上下文信息地址和待切换用户进程上下文信息地址
; ---------------------------------------------------------

[bits 32]
global switchktou,switchCtx
switchktou:
	mov ebp, esp
	mov eax, [ebp+4]						; 原进程的上下文地址
	mov edx, [ebp+8]						; 要切换的进程上下文地址

; 保存原进程上下文
	mov [eax], edi
	mov [eax+4], esi
	mov [eax+8], ebp
	mov [eax+12], esp
	mov [eax+16], ebx
	mov [eax+20], edx
	mov [eax+24], ecx
	mov [eax+28], eax

	mov [eax+32], gs
	mov [eax+36], fs
	mov [eax+40], es
	mov [eax+44], ds

; eip为调用switch前的地址，保存在esp中。返回内核时，从switch后开始执行。
	mov ebx, [esp]
	mov [eax+48], ebx
	mov [eax+52], cs
	pushf
	mov ebx, [esp]
	mov [eax+56], ebx
	popf
	mov [eax+60], esp
	mov [eax+64], ss



; 修改栈中的eflags，使得进入用户进程后打开中断
	sti
	pushf
	mov eax, [esp]
	mov [edx+56], eax
	popf

; 恢复要切换进程的上下文
; edx还要使用，最后恢复
	mov edi, [edx]
	mov esi, [edx+4]
	mov ebp, [edx+8]
	mov ebx, [edx+16]
	mov ecx, [edx+24]
	mov eax, [edx+28]

	mov gs, [edx+32]
	mov fs, [edx+36]
	mov es, [edx+40]
	mov ds, [edx+44]

; 分别压入ss , esp, eflags, cs, eip
	push dword [edx+64]
	push dword [edx+60]
	push dword [edx+56]
	push dword [edx+52]
	push dword [edx+48]

	iret



switchCtx:
	mov ebp, esp
	mov edx, [ebp+4]						; 要切换的上下文

; edx还要使用，最后恢复
	mov edi, [edx]
	mov esi, [edx+4]
	mov ebp, [edx+8]
	mov ebx, [edx+16]
	mov ecx, [edx+24]
	mov eax, [edx+28]

	mov gs, [edx+32]
	mov fs, [edx+36]
	mov es, [edx+40]
	mov ds, [edx+44]

; 分别压入ss , esp, eflags, cs, eip
	push dword [edx+64]
	push dword [edx+60]
	push dword [edx+56]
	push dword [edx+52]
	push dword [edx+48]


	iret



