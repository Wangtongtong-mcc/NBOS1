


void strcopy(char * source, char * dest, int size){
	asm("cld; rep movsb"::"c"(size),"S"(source),"D"(dest));
}

/* strsize用于确认string的大小
 * */
int strsize(char *string){

	int size = 0;
	while ( (*string) != '\0'){
		size++;
		string++;
	}
	return size;

}

/* findlastsprt用于寻找字符串中最后一个分隔符的位置
 * */
int findlastsprt(char * string, char *separator){
	int size = strsize(string);
	char * stringend = string + size;

	// 从字符串结尾开始向前找分隔符
	while ( ((*stringend) != (*separator)) && ((stringend != string))){
		stringend--;
	}
	// 如果到（stringend == string）都没找到分隔符,返回-1
	if (stringend == string && *string  != *separator){
		return -1;
	}
	// 否则返回差值，即最后一个分隔符在string中的位置
	return stringend-string;

}

/* strcuttail用于将string最后一个separator及之后的内容去掉
 * */
//void strcuttail(char *string, char *separator){
//
//	int size = strsize(string);
//	char * stringend = string + size;
//	// 从字符串结尾开始向前找分隔符,没找到原字符串不变
//	while ( (*stringend) != (*separator)){
//		stringend--;
//	}
//	// 若找到，此时stringend即为最后一个分隔符的位置，使其为字符串结尾即可
//	(*stringend) = '\0';
//
//}

/* findfirstsprt用于寻找字符串中第一个分隔符的位置
 * */
int findfirstsprt(char * string, char *separator){

	char * string1 = string;
	int i = 0;

	// 从字符串开头找分隔符
	while ( ((*string1) != (*separator)) && ((*string1) != '\0')){
		string1++;
		i++;
	}
	// 如果直到字符串也没有找到，返回-1
	if((*string1) == '\0'){
		return -1;
	}
	// 否则返回第一个分隔符在string中的位置
	return i;

}

/* strcuthead用于将string第一个separator及之前的内容去掉
// * */
//void strcuthead(char *string, char *separator){
//
//	char * string0 = string;
//	int i = 1;
//	// 从字符串开头找分隔符，没找到原字符串不变
//	while ( (*string0) != (*separator)){
//		string0++;
//		i++;
//	}
//	// 若找到，此时string0即为第一个分隔符的位置，使原字符串指针指向他的下一个字符即可
//	memmove()
//
//}


/* strcut用于截取string第一个separator及之前的内容，将其copy到result中
 * */
void strcut(char *string, char *separator, char *result){

	char * string1 = string;
	int i = 0;
	// 从字符串开头找分隔符，没找到原字符串不变
	while ( ((*string1) != (*separator)) && ((*string1) != '\0')){
		string1++;
		i++;
	}
	// 找到分隔符后，将分隔符前的内容copy到result中
	strcopy(string,result,i+1);

}

/* strcmp用于比较string1和string2是否有size个连续相同字节
 * 相同，返回0；
 * 不相同，string1>string2返回1，否则返回-1；
 * */
int strcmpa(char *string1, char *string2, int size){

	char *a = string1;
	char *b = string2;

	while (size){
		if ((*a) != (*b) ){
			return (*a)>(*b) ? 1 : -1;
		}
		a++;
		b++;
		size--;
	}
	return 0;
}

/* strmatch用于确认dest中是否存在source
 * size为dest大小
 * 存在，返回偏移字节；
 * 不存在，返回-1
 * */
int strmatch(char *source, char *dest,int size){

	int source_size = strsize(source);

	int offset = 0;
	char *string = dest;

	while(offset < size){

		//  先匹配首字母
		while (((*string)!= (*source)) && (offset < size)){
			offset++;
			string++;
		}

		// 若找到相同的首字母，则匹配剩余字符串，即比较两者是否相等
		// 未找到相同首字母，返回-1
		if(offset == size){
			return -1;
		}
		int result = strcmpa(source,string,source_size);
		if(result == 0){
			return offset;
		} else{
			offset++;
			string++;
		}
	}

}

/* strcount用于确认字符串中一共有多少个字符a
 * 存在，返回个数；
 * 不存在，返回0
 * */
int strcount(char *string, char a){
	int count = 0;
	char * start = string;
	while( *start != '\0'){
		if(*start == a){
			count++;
		}
		start++;
	}
	return count;

}