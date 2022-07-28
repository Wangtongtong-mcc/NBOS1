

int xchg(int* s, int i){

	int result;
	asm("xchg %0, %2"\
	:"+m"(*s),"=a"(result)\
	:"a"(i));
	return result;

}