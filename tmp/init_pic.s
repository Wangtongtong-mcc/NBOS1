; init_pic.s用于初始化8259A芯片

[bits 32]
global init_pic
init_pic:

; 初始化主片
	mov al, 0x11
	out 0x20, al							; ICW1：边沿触发，级联，需要写ICW4

	mov al, 0x20
	out 0x21, al							; ICW2: 起始中断向量号为0x20(0-19为处理器内部异常，20-31为Intel保留)

	mov al, 0x04
	out 0x21, al							; ICW3: 主片中IRQ2用于级联从片

	mov al, 0x01
	out 0x21, al 							; ICW4: 8086处理器，非自动结束中断

; 初始化从片
	mov al, 0x11
	out 0xa0, al							; ICW1：边沿触发，级联，需要写ICW4

	mov al, 0x28
	out 0xa1, al							; ICW2: 起始中断向量号为0x28

	mov al, 0x02
	out 0xa1, al							; ICW3: 主片中IRQ2用于级联从片

	mov al, 0x01
	out 0xa1, al 							; ICW4: 8086处理器，非自动结束中断



	ret