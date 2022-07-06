#include "unistd.h"

/* shell为第一个用户进程
 * */
void shell_main(void){
	// 清除屏幕
	clear_my_screen();
	write(STDOUT_FILENO,"I'm shell!\n",12);
	// 输出提示符
	write(STDOUT_FILENO,"shell>\n",7);

//	int fd = creat("/tmp",REGULAR_FILE);
//	write(fd,"How are u?",11);
//	close(fd);
//	int fd0 = open("/tmp",O_RDONLY);
//	char arr [50];
//	read(fd0,arr,3);
//	write(STDOUT_FILENO,arr,3);

	if(fork() == 0){

		char * argv[]={"my","shell"};
		char * envp[]={"your","pro"};
		execve("/mypro",argv,envp);

	} else {

		write(STDOUT_FILENO,"I'm still here!",16);

		while (1){

		};
	}



}