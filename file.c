#include "file.h"
#include "memory.h"
#include "str.h"
#include "print.h"

struct open_file fileList[FILE_MAX_INSYS];

/* write_block用于将block写回磁盘第sector_no扇区
 * */
void write_block(unsigned char * block, int block_no){

	unsigned char arr[512];
	memmov(block, arr, BLOCK_SIZE);
	k_writesector(arr, block_no+SUPER_BLOCK_SECTOR);

}

/* write_superBlock用于在将super_block写回磁盘
 * 位置固定：第300个扇区
 * 
 * */
void write_superBlock(struct super_block * sbptr){

	write_block((unsigned char *)sbptr, SUPER_BLOCK_NO);

}

/* read_superBlock用于在将super_block读到内存sbp指定的地方
 * */
void read_superBlock(struct super_block * sbp){

	unsigned char arr[512];
	k_readsector(arr, SUPER_BLOCK_SECTOR);
	memmov(arr,(unsigned char *)sbp, sizeof(struct super_block));

}

/* read_inode用于在将inode_block_no和inode_offset指定的inode读到内存iptr
 * */
void read_inode(unsigned int inode_block_no, unsigned int inode_offset, struct inode * iptr){
	unsigned char arr[512];
	k_readsector(arr,inode_block_no + SUPER_BLOCK_SECTOR);
	memmov(arr+inode_offset,(unsigned char *)iptr, sizeof(struct inode));
}

/* allocInode用于在磁盘中分配一个新的inode
 * */
int allocInode(struct inode * iptr){

	// 获取super_block
	struct super_block sb;
	read_superBlock(&sb);

	// 从super_block中获取free_inode的头节点
	unsigned int free_inode_block_no = sb.free_inode_block_no;
	unsigned int free_inode_offset = sb.free_inode_offset;

	// 判断并加载空闲inode
	if(!free_inode_block_no){
		return -1;
	}
	read_inode(free_inode_block_no,free_inode_offset,iptr);
	iptr->my_inode_block_no = free_inode_block_no;
	iptr->my_inode_offset = free_inode_offset;
	// 刷新inode中的block_no
	for (int i = 0; i < MAX_BLOCKS_InFILE; i++){
		iptr->block_no[i] = 0;
	}
	// 修改inode中的时间等属性


	// 修改super_block中的free_inode的头节点
	sb.free_inode_offset = iptr->next_inode_offset;
	sb.free_inode_block_no = iptr->next_inode_block_no;
	// 将super_block写回磁盘
	write_superBlock(&sb);
	return 0;

}

/* getFreeBlock用于在位图块中找到空闲块并返回
 * */
unsigned int getFreeBlock(unsigned char *start, int size){

	// 寻找空闲块对应位所在字节
	unsigned char * free_byte_ptr = start;
	int i;
	for (i = 0; i < size; i++){
		if((*free_byte_ptr) != 255){
			break;
		}
		free_byte_ptr++;
	}
	// 寻找空闲块对应位所在位
	unsigned char free_byte = *free_byte_ptr;
	unsigned char index = 0;
	while (free_byte & (1<<index)){
		index++;
	}

	return (unsigned int) (i*8 + index);

}

/* update_dataBlockBit用于更新dataBlock位图，将第block_no个dataBlock更新为status状态
 * */
int update_dataBlockBit(unsigned int block_no, int status){

	// 判断参数有效性
	if ((block_no > FILE_SYS_BLOCKS) || ((status != BLOCK_BUSY) && (status != BLOCK_FREE))){
		return -1;
	}

	// 确认block_no对应bit所在的block
	unsigned int bitBlockNo = block_no /(BLOCK_SIZE * 8) + INODE_BLOCK_SIZE + 1;
	// 将该block读到内存中
	unsigned char arr[512];
	k_readsector(arr,bitBlockNo+SUPER_BLOCK_SECTOR);

	// 确认block_no所在位图中的byte和bit
	int byte = (block_no % (BLOCK_SIZE * 8)) /8;
	int bit = (block_no % (BLOCK_SIZE * 8)) % 8;

	// 更新状态
	if (arr[byte] & (1<<bit)){
		arr[byte] &= (status << bit); // 该位为1
	} else{
		arr[byte] |= (status << bit); // 该位为0
	}


	// 将其写回磁盘
	write_block(arr,bitBlockNo);

	return 0;

}

/* clearBlock用于清空一个block
 * */
int clearBlock(unsigned int block_no){
	unsigned char block[512] = {0};
	write_block(block,block_no);
}


/* allocBlock用于在磁盘中分配一个新的dataBlock，写入到对应的inode中
 * */
int allocBlock(struct inode *iptr){
	// 先确认是否可分配block
	int a = 0;
	while (a < MAX_BLOCKS_InFILE){
		if(iptr->block_no[a] == 0){
			break;
		}
		a++;
	}
	if(a >= MAX_BLOCKS_InFILE){
		return -1;
	}
	// 获取super_block
	struct super_block sb;
	read_superBlock(&sb);

	// 从super_block中获取dataBlockBit的block number
	unsigned int free_block = 0;
	for (int i = 0; i < DataBlockBitSize;i++){
		unsigned int dataBlockBitNo = sb.dataBlockBit_no[i];
		// 将该Block读到内存中
		unsigned char bitBlock[512];
		k_readsector(bitBlock, dataBlockBitNo+SUPER_BLOCK_SECTOR);
		// 取空闲block号
		free_block = getFreeBlock(bitBlock,512);
		// 找到就退出循环，没找到继续循环，在下一个bitBlock中寻找
		if (free_block != -1){
			// getFreeBlock返回的只是在第i个位图块中的空闲位
			free_block = i*512*8 + free_block;
			break;
		}
	}


	// 未找到空闲Block，返回-1
	if(free_block == -1){
		return -1;
	}
	// 否则将block号填到inode中，清空该block，并更新dataBlockBit
	for(int j = 0; j < MAX_BLOCKS_InFILE; j++){
		if (iptr->block_no[j] == 0){
			iptr->block_no[j] = free_block;
			clearBlock(free_block);
			update_dataBlockBit(free_block,BLOCK_BUSY);
			break;
		}
	}
	return 0;

}

/* writeInode用于将iptr指向的inode写回磁盘
 * */
int writeInode(struct inode * iptr){

	// 先将该inode所在block读到内存中
	unsigned char arr[512];
	unsigned int inode_block = iptr->my_inode_block_no;
	k_readsector(arr,inode_block+SUPER_BLOCK_SECTOR);

	// 修改inode所在offset
	memmov((unsigned char *)iptr,arr+iptr->my_inode_offset, sizeof(struct inode));

	// 将该block写回磁盘
	write_block(arr,iptr->my_inode_block_no);

	return 0;

}

/* getRootDirInode用于获取根目录的inode
 * */
int getRootDirInode(struct inode *iptr){

	// 获取super_block
	struct super_block sb;
	read_superBlock(&sb);

	// 从super_block中获取根目录的inode
	unsigned int rootdir_inode_block_no = sb.rootdir_inode_block_no;
	unsigned int rootdir_inode_offset = sb.rootdir_inode_offset;

	// 读入根目录的inode
	read_inode(rootdir_inode_block_no,rootdir_inode_offset,iptr);

	return 0;
}

void init_superBlock(void){

	// 设置free_inode的头指针为第一个空闲的inode
	struct super_block superBlock;
	superBlock.free_inode_block_no = 1;
	superBlock.free_inode_offset = 0;
	// 设置dataBlockBit的DataBlockBitSize个Block号，在INODE_BLOCK后
	for(int i = 0; i < DataBlockBitSize; i++){
		superBlock.dataBlockBit_no[i] = INODE_BLOCK_SIZE + i + 1;
	}
	// 将其写回磁盘
	write_superBlock(&superBlock);

}

void init_inodeList(void){
	// 每个block可存放的inode数量
	int size = sizeof(struct inode);
	int count = BLOCK_SIZE/ size;

	for (int i = 1; i <= INODE_BLOCK_SIZE; i++){
		unsigned char arr[512];
		struct inode * iptr = (struct inode *)arr;
		// 每个block中有count个inode
		for (int j = 0; j < count; j++){
			// 每个inode的block号和offset正好为i和j*size
			iptr->my_inode_block_no = i;
			iptr->my_inode_offset = j*size;
			if (j == (count-1)){
				// 本block最后一个inode下一个inode的block和offset在下一个block开头
				iptr->next_inode_block_no = i + 1;
				iptr->next_inode_offset = 0;
			} else{
				// 每个inode的下一个inode block号和offset为i和(j+1)*size
				iptr->next_inode_block_no = i;
				iptr->next_inode_offset = (j+1)*size;
			}
			iptr++;
		}
		// 单独处理下整个链表中最后一个inode
		if(i == INODE_BLOCK_SIZE){
			// 若判断为最后一个inode，目前iptr指向最后一个inode末尾
			iptr--;
			iptr->next_inode_offset = 0;
			iptr->next_inode_block_no = 0;

		}
		// 将已完成的block写回磁盘
		write_block(arr,i);

	}
}

void init_dataBlockBit(void){
	// 先将所有位置于Free状态
	unsigned char arr[512] = {0};
	for(int i = 1; i <= DataBlockBitSize; i++){
		write_block(arr,INODE_BLOCK_SIZE + i);
	}

	// superBlock, inode, dataBlockBit所占用的 1+INODE_BLOCK_SIZE+DataBlockBitSize个block为Busy状态
	// 上述状态，只更新第一个dataBlockBit即可
	int busyBlock = 1 + DataBlockBitSize + INODE_BLOCK_SIZE;

	// 置从头开始的前byte个字节为0xff
	int byte, bit;
	byte = busyBlock / 8 ;
	for (int j = 0; j < byte; j++){
		arr[j] = 0xff;
	}
	// 置第byte+1个字节的前bit位为busy
	bit = busyBlock % 8;
	for (int k = 0; k < bit; k++){
		arr[byte] += 1<<k;
	}

	// 将第一个dataBlockBit写回磁盘
	write_block(arr,INODE_BLOCK_SIZE + 1);

}

void create_rootDir(){

	// 首先分配一个inode，第一次创建文件，肯定分配的是第一个inode
	struct inode rootDirInode;
	allocInode(&rootDirInode);

	// 再为其分配一个dataBlock
	allocBlock(&rootDirInode);

	rootDirInode.mode = DIRECTORY_FILE;

	// 将根目录的inode写回磁盘
	writeInode(&rootDirInode);

	// 更新superBlock的根目录内容,并最终将其写回磁盘
	struct super_block superBlock;
	read_superBlock(&superBlock);
	superBlock.rootdir_inode_offset = rootDirInode.my_inode_offset;
	superBlock.rootdir_inode_block_no = rootDirInode.my_inode_block_no;
	write_superBlock(&superBlock);

}

void init_userpro(){

	// 为两个进程分配inode
	struct inode in_shell, in_mypro;
	allocInode(&in_mypro);
	allocInode(&in_shell);

	// 为进程填写blocks
	for (int i = 0; i < SHELL_BLOCKS; i++){
		in_shell.block_no[i] = SHELL_SECTOR - SUPER_BLOCK_SECTOR + i;
		update_dataBlockBit(in_shell.block_no[i],BLOCK_BUSY);
	}
	for(int j = 0; j < MYPRO_BLOCKS; j++){
		in_mypro.block_no[j] = MYPRO_SECTOR - SUPER_BLOCK_SECTOR + j;
		update_dataBlockBit(in_shell.block_no[j],BLOCK_BUSY);
	}

	// 补充inode属性
	in_shell.mode = in_mypro.mode = REGULAR_FILE;

	// 将inode写回磁盘
	writeInode(&in_shell);
	writeInode(&in_mypro);

	// 填充根目录
	struct inode root;
	getRootDirInode(&root);

	writeDirectory(&root,"/shell",&in_shell);
	writeDirectory(&root,"/mypro",&in_mypro);

}

void init_openFileList(void){
	for(int i = 0; i < FILE_MAX_INSYS; i++){
		fileList[i].f_inode_block_no = 0;
		fileList[i].f_inode_offset = 0;
		fileList[i].offset = 0;
		fileList[i].isopen = CLOSE;
	}

	// 标准输出属性
	fileList[STDOUT_FILENO].oflags = O_RDWR;
	fileList[STDOUT_FILENO].isopen = OPEN;
	// 标准错误属性
	fileList[STDERR_FILENO].oflags = O_RDWR;
	fileList[STDERR_FILENO].isopen = OPEN;
	// 标准输入属性
	fileList[STDIN_FILENO].oflags = O_RDONLY;
	fileList[STDIN_FILENO].isopen = OPEN;
}

/* init_file用于初始化标准IO的文件结构
 * */
void init_file(void){

	print("Initial FILE SYSTEM!\n");

/* 磁盘布局：
 * No.300扇区为superblock
 * super_block后为inode，共占用连续的INODE_BLOCK_SIZE个block
 * inode后为dataBlockBit所占用的block，共占用连续的DataBlockBitSize个block
 * dataBlockBit管理从superblock开始的一共FILE_SYS_BLOCKS个block，superblock为第0个block
 * dataBlockBit后即可用的datablock
 * */

	// 初始化superblock
	init_superBlock();
	// 初始化inode链表
	init_inodeList();
	// 初始化dataBlock位图
	init_dataBlockBit();
	// 创建根目录
	create_rootDir();
	// 初始化用户进程(目前只有shell和mypro)
	init_userpro();


/* 内存中打开文件的数据结构：
 * 每个进程拥有自己的文件描述符表，存于进程控制块中，有打开标志和指向文件表项的指针两个属性(struct fd)
 * 内核会创建一个全局的文件表，所有在系统中打开的文件都可以在文件表中找到(struct open_file)，
 *  包含打开属性，打开标志，目前的offset，指向inode的指针
 * 每打开一个文件，会从磁盘中将其inode加载到内存中
 * */
	// 初始化标准IO的文件结构,即填写文件表
	init_openFileList();

	print("Initial_fileSYS Done!\n");

}



/* seekNextInode用于确认path的inode
 * root_iptr是path父目录的inode
 * */
int seekNextInode(struct inode * root_iptr, char *path){
	// 获取文件名称
	char file_name[FILE_NAME_MAX];
	strcut(path,"/",file_name);

	// 从根目录中获取它的block，并比对block的内容是否有path
	int offset = -1;
	int i;
	unsigned char block[512];

	for(i = 0; i < MAX_BLOCKS_InFILE; i++){
		// 读入一个block
		k_readsector(block,root_iptr->block_no[i]+SUPER_BLOCK_SECTOR);
		// 在目录中匹配，获取文件名在本block的offset
		offset = strmatch(file_name,block);
		// 找到则退出循环
		if (offset != -1){
			break;
		}
	}

	if(offset == -1){
		return -1;
	}
	// 获取file_name所在的inode
	struct dir_item * dptr = (struct dir_item *)(block + offset);
	unsigned int inode_block_no = dptr->inode_block_no;
	unsigned int inode_offset = dptr->inode_offset;
	// 将其inode读入root_iptr
	read_inode(inode_block_no,inode_offset,root_iptr);
}

/* seekFileInode用于寻找文件path(绝对路径)的inode，并将其存放于iptr
 * */
int seekFileInode(char *path, struct inode *iptr){

	// 获取根目录的inode
	getRootDirInode(iptr);

	// 复制路径到path0，因为可能会修改path
	unsigned char arr[100];
	memmov(path, arr, strsize(path) + 1);
	unsigned char * path0 = &arr[0];


	// 如果path0为根目录，直接返回, 此时iptr已经是根目录的inode
	if(strcmpa(path0,ROOT_DIR,strsize(path0)) == 0){
		return 0;
	}

	// 如果不是根目录文件，去掉path0根节点"/"
	int a = findfirstsprt(path0,"/");
	path0 += a+1;

	// 如果当前path0非空就继续寻找，否则iptr即为最终inode
	while (*path0 != '\0'){
		seekNextInode(iptr,path0);
		if(iptr == NULL){
			return -1;
		}
		a = findfirstsprt(path0,"/");
		if ( a != -1){
			path0 += a+1;
		}else{
			return 0;
		}

	}
	return 0;


}

/* seekBaseDirectory用于确认path的base目录的inode
 * */
int seekBaseDirectory(unsigned char *path,struct inode * base_iptr){

	// 复制路径到path0，因为可能会修改path
	unsigned char path0[100];
	memmov(path, path0, strsize(path) + 1);

	// 去掉文件名
	int lastsprt = findlastsprt(path0,"/");
	if (lastsprt != -1){
		*(path0 + lastsprt) = '\0';
	}



	// 判断是否为根目录下的文件，是根目录的话返回根目录的inode
	if(*path0 == '\0'){
		// 取根目录inode，并返回
		getRootDirInode(base_iptr);
		return 0;
	}

	// 否则去找当前（即去掉文件名称的inode）
	if(seekFileInode(path0,base_iptr) == -1){
		return -1;
	}
	return 0;
}



/* isFree用于判断start到end之间的空间是否是空闲的
 * 空闲返回1，否则返回0
 * */
int isFree(unsigned char * start, unsigned char * end){
	for(int i = 0; i <(end - start); i++){
		if(*(start + i) != 0){
			return 0;
		}
	}
}


/* getFreeOffsetInBlock用于确认blockhead这个空闲offset
 * 参数size表示最少需要的连续空闲字节数
 * 返回得到的offset，没有足够大的空间则返回-1
 * */
int getFreeOffsetInBlock(unsigned char *blockhead, int size){
	unsigned char * ptr = blockhead;
	int offset = 0;
	while (ptr < (blockhead + BLOCK_SIZE)){
		if (isFree(ptr, ptr+size)){
			return offset;
		} else{
			ptr++;
			offset++;
		}
	}
	if (ptr == (blockhead + BLOCK_SIZE)){
		return -1;
	}
}


/* writeDirectory用于在base_inode对应的目录dataBlock中填入文件名和inode号
 * */
int writeDirectory(struct inode *base_inode, unsigned char *path, struct inode *iptr){

	char arr[100];
	memmov(path, arr, strsize(path) + 1);
	char * path0 = &arr[0];


	// 获取文件名称
	int count = strcount(path0, '/');
	for(int i = 0; i < count; i++){

		int a = findfirstsprt(path0,"/");
		if (a != -1){
			path0 += (a + 1);
		}else{
			break;
		}

	}

	// 取上级目录中空闲的block的offset
	int offset = -1;
	int i = 0;
	unsigned char dir_block[512];

	for(i = 0; i < MAX_BLOCKS_InFILE; i++){
		k_readsector(dir_block,base_inode->block_no[i]+SUPER_BLOCK_SECTOR);
		offset = getFreeOffsetInBlock(dir_block, sizeof(struct dir_item));
		// 如果获取到空闲offset, 则退出循环
		if (offset >= 0){
			break;
		}
	}
	// 将文件名和inode号写到offset处
	if(offset >= 0){
		struct dir_item *pos = (struct dir_item *)(dir_block + offset);
		pos->inode_offset = iptr->my_inode_offset;
		pos->inode_block_no = iptr->my_inode_block_no;
		memmov(path0,pos->name,strsize(path0)+1);
	}
	// 将block写回磁盘，block号为base_inode->block_no[i]+SUPER_BLOCK_SECTOR
	write_block(dir_block,base_inode->block_no[i]);

	return 0;

}


/* writeFile用于向file对应的文件的offset处写入ptr，大小为size个字节
 * */
int writeFile(struct open_file * file,struct inode *iptr, void * ptr, unsigned int size){

	// 取当前offset,并判断是否已到文件最大size
	int offset = file->offset;
	if ( (offset + size) > (MAX_BLOCKS_InFILE * BLOCK_SIZE)){
		return -1;
	}

	// 将ptr指向的内容写入
	unsigned char * start = (unsigned char *)ptr;
	unsigned char * end = start + size;

	while (start < end){

		// 取当前offset所对应的block_no
		int index = offset / BLOCK_SIZE;
		unsigned int block_no = iptr->block_no[index];
		if (block_no == 0){
			allocBlock(iptr);
		}

		// 将offset对应的文件dataBlock读到内存中
		unsigned char arr[512];
		k_readsector(arr, block_no + SUPER_BLOCK_SECTOR);

		// 本block还需写入的字节数
		unsigned int left = (BLOCK_SIZE - offset % BLOCK_SIZE) < (end - start) ? (BLOCK_SIZE - offset % BLOCK_SIZE) : (end - start) ;

		// 将ptr内容的left大小写到本block中
		memmov(start, arr+offset%BLOCK_SIZE, left);

		// 将本block写回磁盘
		write_block(arr,block_no);

		// 更新start和offset
		start += left;
		offset += left;

	}

	// 更新file中的offset
	file->offset += size;


	return 0;

}

/* readFile用于向file对应的文件的offset处写入ptr，大小为size个字节
 * */
int readFile(struct open_file * file,struct inode *iptr, void * ptr, unsigned int size){

	// 取当前offset,并判断是否已到文件大小
	int offset = file->offset;
	// 已到文件末尾，返回-1
	if ( offset > iptr->size){
		return -1;
	}

	// 读入文件
	unsigned char * start = (unsigned char *)ptr;
	unsigned char * end = start + size;

	// 读入的字节数
	int readSize = 0;

	while (start < end){
		// 取当前offset所对应的block_no
		int index = offset / BLOCK_SIZE;
		unsigned int block_no = iptr->block_no[index];
		if (block_no == 0){
			return -1;
		}

		// 将offset对应的文件dataBlock读到内存中
		unsigned char arr[512];
		k_readsector(arr, block_no + SUPER_BLOCK_SECTOR);

		// 找到本block中offset对应的位置
		unsigned int posInBlock = offset % BLOCK_SIZE;
		// 本block要读入的字节数
		unsigned int left = (BLOCK_SIZE - posInBlock) < (end - start) ? (BLOCK_SIZE - posInBlock) : (end - start);


		// 将要读的内容复制到指定地方
		memmov(arr+posInBlock,start,left);

		readSize += left;

		// 更新start和offset
		start += left;
		offset += left;
	}

	// 读完后，更新文件表项中的offset
	file->offset += readSize;


	// 返回读入的字节数
	return readSize;


}












/* isopen用于确定进程p是否打开了文件fd
 * */
int isOpen(unsigned int fd, struct pcb * p){

	struct fd current_fd = p->fdlist[fd];
	return current_fd.open_flag;

}

/* haveAccess用于判断已打开的文件file是否有flag权限
 * */
int haveAccess(struct open_file * file, int flag){
	return (file->oflags == flag ? 1 : 0);
}

/* getMinfd用于获取进程p目前最小可用的空闲fd
 * */
int getMinfd(struct pcb * p){
	int fdindex = STDERR_FILENO + 1;
	while (fdindex < FILE_MAX_InPROCESS){
		if (!isOpen(fdindex,p)){
			return fdindex;
		} else{
			fdindex++;
		}
	}
	return -1;
}


/* setOpenFileList用于填充内核的fileList
 * */
struct open_file * setOpenFileList(int oflag,struct inode * iptr){

	int index = -1;
	// 找到目前最小的可用的fileList项index
	for(int i = 0; i < FILE_MAX_INSYS; i++){
		if (fileList[i].isopen == CLOSE){
			index = i;
		}
	}
	if(index == -1){
		return NULL;
	} else{
		// 修改该文件表项的内容
		fileList[index].oflags = oflag;
		fileList[index].isopen = OPEN;
		fileList[index].f_inode_block_no = iptr->my_inode_block_no;
		fileList[index].f_inode_offset = iptr->my_inode_offset;
		// 返回文件表项指针
		return &(fileList[index]);
	}

}

/* delFile用于删除fileList中ofptr指向的文件项
* */
void delFile(struct open_file * ofptr){
	if(ofptr != NULL){
		ofptr->isopen = CLOSE;
		ofptr->offset = 0;
		ofptr->oflags = 0;
		ofptr->f_inode_offset = 0;
		ofptr->f_inode_block_no = 0;
	}

}


