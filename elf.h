#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

struct elf_header{
	unsigned int magic;                                 // ELF魔数，固定为'0x7f' 'E' 'L' 'F'
	unsigned char e_ident[12];
	unsigned short e_type;                              // 文件类型
	unsigned short e_machine;
	unsigned int e_version;
	unsigned int e_entry;                               // 程序入口地址
	unsigned int e_phoff;                               // 程序头表在文件中的偏移地址
	unsigned int e_shoff;
	unsigned int e_flags;
	unsigned short e_ehsize;
	unsigned short e_phentsize;                         // 程序头表中条目大小
	unsigned short e_phnum;                             // 程序头表中条目数量（有几个段）
	unsigned short e_shentsize;
	unsigned short e_shnum;
	unsigned short e_shstmdx;
};

struct program_header{
	unsigned int p_type;                                // 本段类型
	unsigned int p_offset;                              // 本段在文件中的偏移
	unsigned int p_vaddr;                               // 本段在内存中的虚拟地址
	unsigned int p_paddr;                               // 本段在内存中的物理地址
	unsigned int p_filesize;                            // 本段在文件中的大小
	unsigned int p_memsize;                             // 本段在内存中的大小
	unsigned int p_flags;
	unsigned int p_align;
};

#define ELF_MAGIC 0x464c457f
#define LOAD_SEG 1

#endif