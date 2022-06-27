#include "kernel.h"
#include "print.h"

#define KFREE_START 0x140000                            // 0x100000-0x140000中间有内核页表
#define UFREE_START 0x10000000

// 0x00140000为kernel已经使用的1MB内存和页表的空间， KTOTAL_PHY_PAGES为内核可供分配的物理页总数
// UTOTAL_PHY_PAGES为可用于用户进程的物理页总数
#define KTOTAL_PHY_PAGES (UFREE_START - KFREE_START)/PAGESIZE
#define UTOTAL_PHY_PAGES (DEVICE_START - UFREE_START)/PAGESIZE
// 每一个页用一个位来表示，则可以用pages_byte的char数组表示全部空闲页
#define KPAGES_BYTE  KTOTAL_PHY_PAGES/8 + 1
#define UPAGES_BYTE UTOTAL_PHY_PAGES/8 + 1

char kpage_status[KPAGES_BYTE];
char upage_status[UPAGES_BYTE];

// 内核虚拟起始位置为0xc0140000（简单起见）, 同样分配一次修改一次
unsigned int kvir_next_addr = (unsigned int)0xc0140000;

extern unsigned int pagedir[PAGE_STRUCT_ENTRIES];

/* init_memory用于初始化内存位图
 * */
void init_memory(){
	print("Initial memory!\n");
	// 初始化页位图page_status, 初始状态下所有页位空闲
	for(int i = 0; i < KPAGES_BYTE; i++){
		kpage_status[i] = 0;
	}
	for (int i = 0; i < UPAGES_BYTE; i++){
		upage_status[i] = 0;
	}
	print("Init_memory done!\n");
}

/* palloc用于寻找一个空闲物理页，并返回其地址,物理地址空间从0x0010000开始分配
 * */
unsigned int palloc(int type){

	// 确定是内核空间还是用户空间
	char * p = kpage_status;
	unsigned int base = KFREE_START;
	if (type == UMEM){
		p = upage_status;
		base = UFREE_START;
	}

	// 在位图寻找第一个空闲页所在的字节
	unsigned int free_byte = 0;
	// *(p+free_byte) == 0xff说明本字节对应的8个页面全部为已使用状态，继续向下寻找，否则跳出循环
	while(*(p+free_byte) == 0xff){
		free_byte++;
	}

	// 在所在字节中寻找空闲位，找到对应的空闲页
	unsigned int free_bit = 0;
	char free_pages = *(p+free_byte);
	// free_pages & (1<<free_bit)为真说明本位所对应的页为已使用状态，继续向下寻找，否则跳出循环
	while (free_pages & (1<<free_bit)){
		free_bit++;
	}
	// 更改页状态
	*(p+free_byte) += 1<<free_bit;

	// 返回物理页地址
	return (free_byte*8+free_bit)*0x1000+base;
}

/* kmap用于将映射好的虚拟地址和物理页填到内核页结构中
 * 参数为虚拟起始地址和物理页地址
 * */
void kmap(unsigned int vir_addr_start, unsigned int phy_page){
	// 虚拟地址（32位）的高十位为页目录中的索引
	int page_dir_idx = (vir_addr_start & 0xffc00000) >> 22;

	// 从页目录中读取页表对应的pde, 转换为物理地址
	unsigned int page_table = pagedir[page_dir_idx] & 0xfffff000;

	// 虚拟地址的中间十位是页表的索引
	int page_table_idx = (vir_addr_start & 0x003ff000) >> 12;

	// 将物理页地址填入页表对应条目处
	*((unsigned int * )PHY_2_VIR(page_table)+page_table_idx) = phy_page + PAGE_P + PAGE_RW + PAGE_U;
}

/* kalloc用于分配一个内核地址空间的空闲页，返回其地址
 * */
unsigned int * kalloc(){

	// 先在物理空间中寻找一个空闲页
	unsigned int phy_page = palloc(KMEM);
	// 进行映射
	kmap(kvir_next_addr, phy_page);
	// 更新下一个可分配地址
	kvir_next_addr += PAGESIZE;
	// 返回分配的虚拟地址
	return (unsigned int *)(kvir_next_addr - PAGESIZE);
}

/* umap用于将映射好的虚拟地址和物理页填到页结构中
 * 参数为虚拟起始地址和物理页地址
 * */
void umap(unsigned int * page_dir,unsigned int vir_addr_start, unsigned int phy_page){
	// 虚拟地址（32位）的高十位为页目录中的索引
	int page_dir_idx = (vir_addr_start & 0xffc00000) >> 22;
	// 从页目录中读取页表对应的pde
	unsigned int page_table = *(page_dir+page_dir_idx);
	// 若还未创建该页表，需向内核空间申请一个页作为页表
	while (!(page_table & PAGE_P)){
		// 申请后，将页表对应的pde填入到页目录对应的位置
		page_table = VIR_2_PHY((unsigned int)kalloc())+ PAGE_P + PAGE_RW + PAGE_U;
		*(page_dir+page_dir_idx) = page_table;
	}

	// 虚拟地址的中间十位是页表的索引
	int page_table_idx = (vir_addr_start & 0x003ff000) >> 12;

	// 将物理页地址填入页表对应条目处
	unsigned int * pte = (unsigned int *)PHY_2_VIR((page_table & 0xfffff000))+page_table_idx;
	*pte = phy_page + PAGE_P + PAGE_RW + PAGE_U;
}

/* ualloc用于分配一个用户地址空间的空闲页，返回其地址
 * */
unsigned int * ualloc(unsigned int * uvir_next_addr,unsigned int * upage_dir){

	// 先在物理空间中寻找一个空闲页与其映射
	unsigned int phy_page = palloc(UMEM);
	// 进行映射，映射到用户程序的页目录中
	umap(upage_dir, *uvir_next_addr, phy_page);
	// 更新下一个可分配地址
	*uvir_next_addr += PAGESIZE;
	// 返回分配的虚拟地址
	return (unsigned int *)(*uvir_next_addr - PAGESIZE);
}







/* get_phy_addr用于获取某指定虚拟地址的物理地址
 * */
unsigned int * get_phy_addr(unsigned int * vir_addr){
	// 虚拟地址（32位）的高十位为页目录中的索引
	unsigned int vir_addr_start = (unsigned int )vir_addr;
	int page_dir_idx = (vir_addr_start & 0xffc00000) >> 22;
	// 虚拟地址的中间十位是页表的索引
	int page_table_idx = (vir_addr_start & 0x003ff000) >> 12;
	// 页表
	unsigned int pt = *((unsigned int *)(&pagedir) + page_dir_idx);
	unsigned int * page_table = (unsigned int * )(PHY_2_VIR(pt & 0xfffff000));
	// 物理地址
	return (unsigned int * )(*(page_table+page_table_idx) & 0xfffff000);
}


//* alloc用于分配size大小的内存，返回指向该内存块(虚拟内存)的指针
// * */
//unsigned char * alloc(unsigned long size, int type){
//	// 确定要分配的页的数量
//	int page_num;
//	if (size % PAGE_SIZE == 0){
//		page_num =  size / PAGE_SIZE;
//	} else{
//		page_num = size / PAGE_SIZE + 1;
//	}
//
//	// 分配的虚拟内存以vir_addr_start起，vir_addr_end结束
//	unsigned int vir_addr_start;
//	// 确定起始虚拟地址
//	if(type == KMEM){
//		vir_addr_start = kvir_next_addr;
//	} else{
//		vir_addr_start = uvir_next_addr;
//	}
//	unsigned int vir_addr_end = vir_addr_start + size;
//
//	// 分配物理页并进行映射
//	while(vir_addr_start < vir_addr_end)){
//		unsigned char * phy_page = palloc();
//		if (!phy_page){
//			return NULL;
//		}
//		// 填充页结构,并为下一个虚拟页地址映射物理页
//		put_a_page(vir_addr_start,(unsigned int)phy_page);
//		vir_addr_start += PAGE_SIZE;
//	}
//
//	// 分配成功，更新下一个可分配的虚拟起始地址
//	next_vir_addr = vir_addr_end;
//	// 分配成功，并将此次分配存放到mem_alloc_record中
//	// put_a_record(vir_addr_start,size);
//
//	return (void*)(vir_addr_end-size);
//
//}

/* memcopy用于将source处size大小的内存块复制到dest处
 * */
void memcopy(unsigned char * source, unsigned char * dest, unsigned int size){
	for(int i = 0; i < size; i++){
		*(dest + i)= *(source + i);
	}

}

/* memmov用于将source处size大小的内存块移动到dest处
 * */
void memmov(unsigned char * source, unsigned char * dest, unsigned int size){
	asm("cld; rep movsb"::"c"(size),"S"(source),"D"(dest));
}