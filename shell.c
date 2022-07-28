#include "unistd.h"

/* getPath为确认是否command为正确的命令
 * 若正确，将其路径返回到path中，否则返回-1
 * */
int getPath(char *command, char *path){

	path[0] = '/';
	path[1] = 'm';
	path[2] = 'y';
	path[3] = 'p';
	path[4] = 'r';
	path[5] = 'o';
	path[6] = '\n';
	return 0;

}

/* shell为第一个用户进程
 * */
void shell_main(void){
	// 清除屏幕
	clear_my_screen();
	write(STDOUT_FILENO,"I'm shell!\n",12);
	// 输出提示符
	write(STDOUT_FILENO,"Shell>",7);

	// 等待用户输入
	while(1){
		char command[30];

		int size = read(STDIN_FILENO,command,30);
		if ( size > 0 ){

			char path[30];
			// 在环境变量中找到是否有命令字符串
			if(getPath(command,path) == -1){
				write(STDIN_FILENO,"Command not found!\n",20);
			}
			// 命令若存在，就新fork一个进程，去执行该命令
			// 否则继续循环等待输入
			if(fork() == 0){
				char * argv[]={"my","shell"};
				char * envp[]={"your","pro"};
				execve(path,argv,envp);
			}
			int * statloc;
			if(wait(statloc) > 0){
				write(STDIN_FILENO,"Shell>",8);
			};

		}

	}


//	if(fork() == 0){
//
//
//		execve("/mypro",argv,envp);
//
//	} else {
//
//		write(STDOUT_FILENO,"I'm still here!\n",17);
//	}
//
//
//	write(STDOUT_FILENO,"Shell Again!\n",14);





}