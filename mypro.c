#include "unistd.h"

int main(void){

	clear_my_screen();
	write(STDOUT_FILENO,"HI,I'm user!\n",14);

	_exit(0);
}