[bits 32]

global divide_error_handler,debug_handler,nmi_handler,breakpoint_handler,overflow_handler,bound_handler
global invalid_opcode_handler,double_fault_handler,invalid_tss_handler,segment_not_present_handler
global stack_segment_handler,general_protection_handler,page_fault_handler,syscall_handler,other_exception_handler
global timer_handler,keyboard_handler

extern interrupt_assign,print

interrupt_entry:
; 保存上下文

	push ds
	push es
	push fs
	push gs
	pusha

	mov ebp, esp

; 	切换段选择子
	mov ax, 0x10						; 更换为内核数据段选择子
	mov ds, ax
	mov es, ax

; 由于设置了非自动结束中断，需手动向8259A芯片发送EOI
; 如果是从片上进入的中断,除了往从片上发送EOI外,还要往主片上发送EOI
; 中断结束命令，即设置OCW2中EOI位为1
	mov al,0x20 					; 中断结束命令EOI
	out 0xa0,al 					; 向从片发送
	out 0x20,al						; 向主片发送

; 压入参数，调用中断分派函数
	push ebp
	call interrupt_assign

;根据中断号，确认是否需要更新eax
	cmp dword [ebp+12*4], 0x80
	jne not80
;保存中断结果
	mov [ebp+7*4], eax

; 恢复环境
not80:
	add esp, 4						; interrupt_assign的参数

	popa
	pop gs
	pop fs
	pop es
	pop ds

	add esp, 8						; 向量号和中断错误码

; 中断返回
	iret


; -----------------------------------------------------------------------------------------
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
	push 0						   ; 伪Error Code
	push 0						   ; 中断向量号
	jmp interrupt_entry


; debug_handler是用于处理debug
; Vector No.:1 ; Mnemonic: #DB; Source: Any code or data reference;
; Type: Fault/Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
debug_handler:
	push 0						   ; 伪Error Code
	push 1						   ; 中断向量号
	jmp interrupt_entry


; nmi_handler是用于处理不可屏蔽异常的
; Vector No.:2 ; Mnemonic: /; Source: Non-maskable external interrupt;
; Type: Interrupt; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
nmi_handler:
	push 0						   ; 伪Error Code
	push 2						   ; 中断向量号
	jmp interrupt_entry


; breakpoint_handler是用于处理由int 3指令引发的中断
; Vector No.:3 ; Mnemonic: #BP; Source: INT3 instruction;
; Type: Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
breakpoint_handler:
	push 0						   ; 伪Error Code
	push 3						   ; 中断向量号
	jmp interrupt_entry


; overflow_handler是用于处理溢出错误引发的中断
; Vector No.:4 ; Mnemonic: #OF; Source: INTO instruction;
; Type: Trap; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
overflow_handler:
	push 0						   ; 伪Error Code
	push 4						   ; 中断向量号
	jmp interrupt_entry


; bound_handler是用于处理边界检查出错引发的中断
; Vector No.:5 ; Mnemonic: #BR; Source: BOUND instruction;
; Type: Fault; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
bound_handler:
	push 0						   ; 伪Error Code
	push 5						   ; 中断向量号
	jmp interrupt_entry


; invalid_opcode_handler是用于处理无效操作码引发的中断
; Vector No.:6 ; Mnemonic: #UD; Source: UD2 instruction or reserved opcode.1;
; Type: Fault; No Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
invalid_opcode_handler:
	push 0						   ; 伪Error Code
	push 6						   ; 中断向量号
	jmp interrupt_entry


; double_fault_handler是用于处理双错误故障
; Vector No.:8 ; Mnemonic: #DF; Source: Any instruction that can generate an exception, an NMI, or an INTR;
; Type: Abort; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
double_fault_handler:
	push 8						   ; 中断向量号
	jmp interrupt_entry


; invalid_tss_handler是用于处理无效tss
; Vector No.:10 ; Mnemonic: #TS; Source:Task switch or TSS access;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
invalid_tss_handler:
	push 10						   ; 中断向量号
	jmp interrupt_entry


; segment_not_present_handler是用于处理段不存在引发的中断
; Vector No.:11 ; Mnemonic: #NP; Source: Loading segment registers or accessing system segmnets;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
segment_not_present_handler:
	push 11						   ; 中断向量号
	jmp interrupt_entry


; stack_segment_handler是用于处理栈段错误
; Vector No.:12 ; Mnemonic: #SS; Source:Stack operations and SS register loads;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
stack_segment_handler:
	push 12					    ; 中断向量号
	jmp interrupt_entry


; general_protection_handler是用于处理一般性保护错误
; Vector No.:13 ; Mnemonic: #GP; Source:Any memory reference and other protection checks;
; Type: Fault; Error Code
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP,Error Code
; ------------------------------------------------------
general_protection_handler:
	push 13				    ; 中断向量号
	jmp interrupt_entry


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
	jmp interrupt_entry


; timer_handler用于处理时钟中断
; Vector No.:0x20;
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
timer_handler:
	push 0
	push 0x20
	jmp interrupt_entry

; timer_handler用于处理时钟中断
; Vector No.:0x20;
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
keyboard_handler:
	push 0
	push 0x21
	jmp interrupt_entry

; syscall_handler用于处理系统调用
; Vector No.:0x80 ;
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; 需根据eax中的功能号执行具体的系统函数
; ------------------------------------------------------
syscall_handler:
	push 0
	push 0x80
	jmp interrupt_entry





; other_exception_handler用于处理除上述的其他中断
; 发生中断后，处理器依次在栈中压入SS,ESP,EFLAGS,CS,EIP
; ------------------------------------------------------
other_exception_handler:
	push 0						   ; 伪Error Code
	push 257					   ; 伪中断向量号
	jmp interrupt_entry


msg:
	db 'Other Page Fault Occur!'
msg1:
	db 'Timer'


