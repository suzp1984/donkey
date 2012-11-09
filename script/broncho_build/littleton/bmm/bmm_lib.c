/*
 *  bmm_lib.c
 *
 *  Buffer Management Module
 *
 *  User level BMM Defines/Globals/Functions
 *
 *  Li Li (lea.li@marvell.com)

 *(C) Copyright 2007 Marvell International Ltd.
 * All Rights Reserved
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "bmm_lib.h"

//#define DEBUG

#ifdef DEBUG
  #define pr_debug(fmt, arg...) printf(fmt, ##arg)
#else
  #define pr_debug(fmt, arg...) do { } while(0)
#endif

static int bmm_fd = 0;
typedef struct {
	unsigned long input;		/* the starting address of the block of memory */
	unsigned long output;		/* the starting address of the block of memory */
	unsigned long length;		/* the length of the block of memory */
} ioctl_arg_t;

int bmm_init()
{
        /* attempt to open the BMM driver */
	if(bmm_fd <= 0)
		bmm_fd = open(BMM_DEVICE_FILE, O_RDWR);

	/* if the open failed, try to mount the driver */
	if(bmm_fd < 0 ) {
		system("mknod /dev/bmm c 10 94\n");
		bmm_fd = open(BMM_DEVICE_FILE, O_RDWR);
	}

	pr_debug("BMM device fd: %d", bmm_fd);

	return bmm_fd;
}

void bmm_exit()
{
	if(bmm_fd > 0)
		close(bmm_fd);
	bmm_fd = 0;
}

void *bmm_malloc(unsigned long size, int attr)
{
	int ret;
	void *vaddr;
	ioctl_arg_t io;

	if(size == 0)
		return NULL;

	if(bmm_init() < 0)
		return NULL;

	io.input = size;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_MALLOC, attr), &io);
	if(ret < 0 || io.output == NULL)
		return NULL;

	pr_debug("bmm_malloc return paddr = 0x%08lx\n", io.output);

	vaddr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, bmm_fd, io.output);

	return ((int)vaddr == -1) ? NULL : vaddr;
}

void bmm_free(void *vaddr)
{
	unsigned long size;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return;

	size = bmm_get_mem_size(vaddr);
	munmap(vaddr, size);

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ioctl(bmm_fd, BMM_IOCTL(BMM_FREE, 0), &io);
}

void *bmm_get_vaddr(void *paddr)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return NULL;

	io.input = (unsigned long)paddr;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_VIRT_ADDR, 0), &io);
	if(ret < 0)
		return NULL;

	return (void *)io.output;
}

void *bmm_get_paddr(void *vaddr)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return NULL;

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_PHYS_ADDR, 0), &io);
	if(ret < 0)
		return NULL;

	return (void *)io.output;
}

unsigned long bmm_get_mem_size(void *vaddr)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_MEM_SIZE, 0), &io);
	if(ret < 0)
		return 0;

	return io.output;
}

int bmm_get_mem_attr(void *vaddr)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_MEM_ATTR, 0), &io);
	if(ret < 0)
		return 0;

	return (int)io.output;
}

int bmm_set_mem_attr(void *vaddr, int attr)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_SET_MEM_ATTR, attr), &io);
	if(ret < 0)
		return 0;

	return (int)io.output;
}

unsigned long bmm_get_total_space()
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = 0;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_TOTAL_SPACE, 0), &io);
	if(ret < 0)
		return 0;

	return io.output;
}

unsigned long bmm_get_free_space()
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = 0;
	io.output = 0;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_GET_FREE_SPACE, 0), &io);
	if(ret < 0)
		return 0;

	return io.output;
}

void bmm_flush_cache(void *vaddr, int dir)
{
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return;

	io.input = (unsigned long)vaddr;
	io.output = 0;
	ioctl(bmm_fd, BMM_IOCTL(BMM_FLUSH_CACHE, dir), &io);
}

void *bmm_dma_memcpy(void *dest, const void *src, size_t n)
{
	int ret;
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return 0;

	io.input = (unsigned long)src;
	io.output = (unsigned long)dest;
	io.length = (unsigned long)n;
	ret = ioctl(bmm_fd, BMM_IOCTL(BMM_DMA_MEMCPY, 0), &io);
	if(ret < 0)
		return 0;

	return dest;
}

void bmm_dma_sync()
{
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return;

	io.input = 0;
	io.output = 0;
	ioctl(bmm_fd, BMM_IOCTL(BMM_DMA_SYNC, 0), &io);
}

void bmm_flush_cache_range(void *start, size_t size, int direction)
{
	ioctl_arg_t io;

	if(bmm_init() < 0)
		return;

	io.input = (unsigned long)start;
	io.length = size;
	io.output = 0;
	ioctl(bmm_fd, BMM_IOCTL(BMM_CONSISTENT_SYNC, direction), &io);
}

