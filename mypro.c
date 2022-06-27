#include "unistd.h"

void main(void){
	write(STDOUT_FILENO,"HI,I'm user!\n",14);
	while (1){

	}
}