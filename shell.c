#include "unistd.h"

/* shell为第一个用户进程
 * */
void shell_main(void){
	// 清除屏幕
	clear_my_screen();
	write(STDOUT_FILENO,"I'm shell!\n",12);
	// 输出提示符
	write(STDOUT_FILENO,"shell>\n",7);

	int fd = creat("/shell",REGULAR_FILE);
	write(fd,"Hi, shell! Ni hao! Haaaaa",26);
	close(fd);
	open("/shell",O_RDONLY);
	char arr [50];
	read(fd,arr,3);
	write(STDOUT_FILENO,arr,3);


	while (1){

	};
}