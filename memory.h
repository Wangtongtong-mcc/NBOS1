#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H


void init_memory(void);
unsigned int * get_phy_addr(unsigned int * vir_addr);
unsigned int * kalloc();
unsigned int * ualloc(unsigned int * uvir_next_addr,unsigned int * upage_dir);
unsigned int palloc(int type);
void kmap(unsigned int vir_addr_start, unsigned int phy_page);
void umap(unsigned int * pagedir, unsigned int vir_addr_start, unsigned int phy_page);
void memcopy(unsigned char * source, unsigned char * dest, unsigned int size);
void memmov(unsigned char * source, unsigned char * dest, unsigned int size);


#endif