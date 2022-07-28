#ifndef _KERNEL_MEMORY_H
#define _KERNEL_MEMORY_H

#include "kernel.h"

void init_memory(void);
//unsigned int * get_phy_addr(unsigned int * vir_addr);
//unsigned int * kalloc();
unsigned int * ualloc(unsigned int * uvir_next_addr,unsigned int * upage_dir);
//unsigned int palloc(int type);
//void kmap(unsigned int vir_addr_start, unsigned int phy_page);
//void umap(unsigned int * pagedir, unsigned int vir_addr_start, unsigned int phy_page);
void memcopy(unsigned char * source, unsigned char * dest, unsigned int size);
void memmov(unsigned char * source, unsigned char * dest, unsigned int size);

void free_page(struct page_t * page);

unsigned char * alloc_page();
void page_map(struct page_t * vir_addr,struct page_t * phy_addr,unsigned int type);
void fill(unsigned char * ptr, unsigned int size, unsigned char data);
void generate_page_table_item(struct page_table_item_t * page_table_item_ptr, struct page_t * page_phy_ptr, unsigned int page_privilege);
void generate_page_dir_item(struct page_dir_item_t * page_dir_item_ptr, struct page_table_t * page_table_phy_ptr, unsigned int page_privilege);


#endif