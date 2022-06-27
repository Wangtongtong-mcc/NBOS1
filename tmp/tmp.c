/* no_code_handler为统一的无错误码处理函数
 * 打印中断向量号，中断名称
 * 发生该中断的函数eip,eflags,ss, esp等
 * */
void no_code_handler(unsigned int vectorno, unsigned int esp_ptr){
	unsigned int * esp = (unsigned int *)esp_ptr;
	// 分别打印中断向量，异常名称
	print("Exception without Error Code Occur!\n");
	print("Vector NO.:");
	printn(vectorno);
	print("\nException name:");
	print(intr_name[vectorno]);
	// 打印发生中断时的CS，EIP，EFLAGS
	print("\nCS  EIP  EFLAGS\n");
	printn(esp[1]);
	print("  ");
	printn(esp[0]);
	print("  ");
	printn(esp[2]);

	asm("hlt");
}

/* error_code_handler为统一的有错误码处理函数
 * 打印中断向量号，中断名称, 错误码等
 * 发生该中断的函数eip,eflags,ss, esp等
 * */
void error_code_handler(unsigned int vectorno, unsigned int errcode, unsigned int esp_ptr){
	unsigned int * esp = (unsigned int *)esp_ptr;
	// 分别打印中断向量，异常名称，错误码
	print("Exception with Error Code Occur!\n");
	print("Vector NO.:");
	printn(vectorno);
	print("\nException name:");
	print(intr_name[vectorno]);
	print("\nError Code:");
	printn(errcode);
	// 打印发生中断时的CS，EIP，EFLAGS
	print("\nCS  EIP  EFLAGS\n");
	printn(esp[1]);
	print("  ");
	printn(esp[0]);
	print("  ");
	printn(esp[2]);

	asm("hlt");
}