
[bits 32]
extern print,no_code_handler,error_code_handler,page_np_handler,sys_call_table
; 通用函数
global general_intr_handler
; 内部异常
global divide_error_handler,debug_handler,nmi_handler,breakpoint_handler,overflow_handler,bound_handler
global invalid_opcode_handler,double_fault_handler,invalid_tss_handler,segment_not_present_handler
global stack_segment_handler,general_protection_handler,page_fault_handler,syscall_handler


; general_nocode_handler是处理无错误码的异常的通用程序
; 进入中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
general_nocode_handler:
; 保存环境
	push ds
	push es
	push fs
	push gs
	pushad

	mov ebp, esp

	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

; 由于设置了非自动结束中断，需手动向8259A芯片发送EOI
; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI
; 中断结束命令，即设置OCW2中EOI位为1
	mov al,0x20 					; 中断结束命令EOI
	out 0xa0,al 					; 向从片发送
	out 0x20,al						; 向主片发送

	lea edx, [ebp+13*4]				; 指向返回地址
	push edx
	push dword [ebp+12*4]			; 中断向量号
	call no_code_handler

; 恢复环境
	add esp, 8						; 跳过no_code_handler的参数
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4						; 跳过中断向量号
	iret


; general_withcode_handler是处理有错误码的异常的通用程序
; 进入中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,ERROR CODE
; ------------------------------------------------------
general_withcode_handler:
; 保存环境
	push ds
	push es
	push fs
	push gs
	pushad

	mov ebp, esp

	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

; 由于设置了非自动结束中断，需手动向8259A芯片发送EOI
; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI
; 中断结束命令，即设置OCW2中EOI位为1
	mov al,0x20                   ; 中断结束命令EOI
	out 0xa0,al                   ; 向从片发送
	out 0x20,al                   ; 向主片发送

	lea edx, [ebp+14*4]				; 指向返回地址
	push edx
	push dword [ebp+13*4]			; Errorcode
	push dword [ebp+12*4]			; 中断向量号
	call error_code_handler

; 恢复环境
	add esp, 12					  ; 跳过error_code_handler数的参数
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 8					   ; 跳过向量号和ERROR CODE
	iret

; int9 386处理器后不会出现该异常
; int17 由486处理器引入
; int18 由奔腾处理器引入，P6系强化
; int19 由奔腾3引入

; divide_error_handler是用于处理处理被零除出错的情况
; Vector No.:0 ; Mnemonic: #DE; Source: DIV and IDIV instructions;
; Type: Fault; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
divide_error_handler:
	push 0						   ; 中断向量号
	jmp general_nocode_handler


; debug_handler是用于处理debug
; Vector No.:1 ; Mnemonic: #DB; Source: Any code or data reference;
; Type: Fault/Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
debug_handler:
	push 1						   ; 中断向量号
	jmp general_nocode_handler


; nmi_handler是用于处理不可屏蔽异常的
; Vector No.:2 ; Mnemonic: /; Source: Non-maskable external interrupt;
; Type: Interrupt; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
nmi_handler:
	push 2						   ; 中断向量号
	jmp general_nocode_handler


; breakpoint_handler是用于处理由int 3指令引发的中断
; Vector No.:3 ; Mnemonic: #BP; Source: INT3 instruction;
; Type: Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
breakpoint_handler:
	push 3						   ; 中断向量号
	jmp general_nocode_handler


; overflow_handler是用于处理溢出错误引发的中断
; Vector No.:4 ; Mnemonic: #OF; Source: INTO instruction;
; Type: Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
overflow_handler:
	push 4						   ; 中断向量号
	jmp general_nocode_handler


; bound_handler是用于处理边界检查出错引发的中断
; Vector No.:5 ; Mnemonic: #BR; Source: BOUND instruction;
; Type: Fault; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
bound_handler:
	push 5						   ; 中断向量号
	jmp general_nocode_handler


; invalid_opcode_handler是用于处理无效操作码引发的中断
; Vector No.:6 ; Mnemonic: #UD; Source: UD2 instruction or reserved opcode.1;
; Type: Fault; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
invalid_opcode_handler:
	push 6						   ; 中断向量号
	jmp general_nocode_handler


; double_fault_handler是用于处理双错误故障
; Vector No.:8 ; Mnemonic: #DF; Source: Any instruction that can generate an exception, an NMI, or an INTR;
; Type: Abort; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
double_fault_handler:
	push 8						   ; 中断向量号
	jmp general_withcode_handler


; invalid_tss_handler是用于处理无效tss
; Vector No.:10 ; Mnemonic: #TS; Source:Task switch or TSS access;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
invalid_tss_handler:
	push 10						   ; 中断向量号
	jmp general_withcode_handler


; segment_not_present_handler是用于处理段不存在引发的中断
; Vector No.:11 ; Mnemonic: #NP; Source: Loading segment registers or accessing system segmnets;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
segment_not_present_handler:
	push 11						   ; 中断向量号
	jmp general_withcode_handler


; stack_segment_handler是用于处理栈段错误
; Vector No.:12 ; Mnemonic: #SS; Source:Stack operations and SS register loads;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
stack_segment_handler:
	push 12					    ; 中断向量号
	jmp general_withcode_handler


; general_protection_handler是用于处理一般性保护错误
; Vector No.:13 ; Mnemonic: #GP; Source:Any memory reference and other protection checks;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
general_protection_handler:
	push 13				    ; 中断向量号
	jmp general_withcode_handler


; page_fault_handler是用于处理页面错误
; Vector No.:14 ; Mnemonic: #PF; Source:Any memory reference;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; #PF的错误码格式：
; 31               15		   5 4 3 2 1 0
; -----------------------------------------
; |    Reserved    | |Reserved| | | | | |P|
; -----------------------------------------
; P 0 由缺页造成.
; 	1 由其他页面级保护违规造成.
; W/R 0 访问页面为读操作.
;	  1 访问页面为写操作.
; U/S 0 管理员模式.
;	  1 用户模式.
; RSVD 1 设置了分页结构中的保留位.
; I/D 1 取指时发生错误.
; PK 1 访问用户模式地址导致的错误.
; SGX 1 异常与分页无关，由于违反SGX特定的访问控制要求而导致的.
; ------------------------------------------------------
page_fault_handler:
	push 14				    ; 中断向量号
	push ds					; 保存环境
	push es
	push fs
	push gs
	pushad

	mov ebp, esp

	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

	mov eax, [ebp+14*4]					; 错误码
	mov edx, cr2						; 引起页面异常的线性地址
	push eax
	push edx

	test eax, 1							; 是否为缺页异常
	jne other_pf
	call page_np_handler
	jmp return
other_pf:
	push 0								; 为保证page_np_handler函数和print函数的参数个数相同
	push msg2
	call print
return:
	add esp, 8							; 跳过page_np_handler函数或print函数的参数
	popad								; 恢复环境
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 8							; 跳过向量号和错误码
	iret


; syscall_handler是用于处理系统调用
; Vector No.:0x80;
syscall_handler:
; 保存环境
	push ds
	push es
	push fs
	push gs
	pusha


; 参数压栈
; ebx为第一个参数，ecx为第二个参数，edx为第三个参数
	mov ebp, eax

	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

	push edx
	push ecx
	push ebx

; 调用结果存放于eax中
	mov eax, [sys_call_table + 4*ebp]
	call eax
	pop ebx
	pop ecx
	pop edx

; 修改栈中的eax
	mov [esp + 7*4], eax

; 恢复环境
	popa
	pop gs
	pop fs
	pop es
	pop ds

	iret


; general_intr_handle是用于初始化idt的通用函数，无code
; 进入中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
;----------------------------------------------------------------
general_intr_handler:
; 保存环境
	push ds
	push es
	push fs
	push gs
	pushad

	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

; 由于设置了非自动结束中断，需手动向8259A芯片发送EOI
; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI
; 中断结束命令，即设置OCW2中EOI位为1
	mov al,0x20                   ; 中断结束命令EOI
	out 0xa0,al                   ; 向从片发送
	out 0x20,al                   ; 向主片发送

	push msg1
	call print

; 恢复环境
	add esp, 4					  ; 跳过打印函数的参数
	popad
	pop gs
	pop fs
	pop es
	pop ds
	iret

msg1:
	db 'Some Unkonwn Exception Occur!'
msg2:
	db 'Other Page Fault Occur!'
