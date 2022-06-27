#ifndef _X86_H
#define _X86_H

#define outb(port, data) \
	asm("outb %b0, %w1": :"a"(data),"Nd"(port))

#define outw(port, data) \
	asm("outw %w0, %w1": :"a"(data),"Nd"(port))

#define inb(port,data) \
	asm("inb %w1, %b0": "=a"(data): "Nd"(port))

#define inw(port,data) \
	asm("inw %w1, %0": "=a"(data): "Nd"(port))

// 磁盘端口
#define IDE_DATA_PORT 0x1f0
#define IDE_SECTOR_COUNT_PORT 0x1f2
#define IDE_SECTOR_NUMBER_PORT 0x1f3
#define IDE_CYLINDER_LOW_PORT 0x1f4
#define IDE_CYLINDER_HIGH_PORT 0x1f5
#define IDE_DEVICE_PORT 0x1f6
#define IDE_STATUS_PORT 0x1f7

#define VEDIO_INDEX_PORT 0x03d4
#define VEDIO_DATD_PORT 0x03d5
#define HIGH_CURSOR_INDEX 0x0e
#define LOW_CURSOR_INDEX 0X0f

#define READ_DISK_FLAG 0
#define WRITE_DISK_FLAG 1

// 8259a芯片端口
#define PIC_M1 0x20
#define PIC_M2 0x21
#define PIC_S1 0xa0
#define PIC_S2 0xa1


#endif







