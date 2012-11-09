/*
 *  bmm_lib.h
 *
 *  Buffer Management Module
 *
 *  User level BMM Defines/Globals/Functions
 *
 *  Li Li (lea.li@marvell.com)

 *(C) Copyright 2007 Marvell International Ltd.
 * All Rights Reserved
 */

#if !defined (__BMM_LIB_H__)
#define __BMM_LIB_H__

#define BMM_MINOR		(94)
#define BMM_MEMORY_SIZE		(10 * 1024 * 1024)

/* assemble an ioctl command */
#define BMM_IOCTL(cmd, arg)	(((cmd) << 16) | (arg))

/* disassemble an ioctl command */
#define BMM_IOCTL_CMD(cmd)	((cmd) >> 16)
#define BMM_IOCTL_ARG(cmd)	((cmd) & 0xffff)

/* ioctl commands */
#define BMM_MALLOC		(0)
#define BMM_FREE		(1)
#define BMM_GET_VIRT_ADDR	(2)
#define BMM_GET_PHYS_ADDR	(3)
#define BMM_GET_MEM_ATTR	(4)
#define BMM_SET_MEM_ATTR	(5)
#define BMM_GET_MEM_SIZE	(6)
#define BMM_GET_TOTAL_SPACE	(7)
#define BMM_GET_FREE_SPACE	(8)
#define BMM_FLUSH_CACHE		(9)
#define BMM_DMA_MEMCPY		(10)
#define BMM_DMA_SYNC		(11)
#define BMM_CONSISTENT_SYNC	(12)

/* ioctl arguments: memory attributes */
#define	BMM_ATTR_DEFAULT	(0)		/* noncacheable nonbufferable */
#define	BMM_ATTR_HUGE_PAGE	(1 << 0)	/* 64KB page size (not supported yet) */
#define	BMM_ATTR_WRITETHROUGH	(1 << 1)	/* implies L1 Cacheable (not supported yet) */
#define BMM_ATTR_BUFFERABLE	(1 << 2)	/* matches PTE (not supported yet) */
#define BMM_ATTR_CACHEABLE	(1 << 3)	/* matches PTE (not supported yet) */
#define BMM_ATTR_L2_CACHEABLE	(1 << 4)	/* implies L1 Cacheable (not supported yet) */

/* ioctl arguments: cache flush direction */
#define BMM_DMA_BIDIRECTIONAL	(0)		/* DMA_BIDIRECTIONAL */
#define BMM_DMA_TO_DEVICE	(1)		/* DMA_TO_DEVICE */
#define BMM_DMA_FROM_DEVICE	(2)		/* DMA_FROM_DEVICE */
#define BMM_DMA_NONE		(3)		/* DMA_NONE */

#define BMM_DEVICE_FILE	"/dev/bmm"

#if defined (__cplusplus)
extern "C" {
#endif

int bmm_init();
void bmm_exit();
void *bmm_malloc(unsigned long size, int attr);
void bmm_free(void *vaddr);
void *bmm_get_vaddr(void *paddr);
void *bmm_get_paddr(void *vaddr);
int bmm_get_mem_attr(void *vaddr);		/* Not supported yet */
int bmm_set_mem_attr(void *vaddr, int attr);	/* Not supported yet */
unsigned long bmm_get_mem_size(void *vaddr);
unsigned long bmm_get_total_space();
unsigned long bmm_get_free_space();
void bmm_flush_cache(void *vaddr, int dir);
void *bmm_dma_memcpy(void *dest, const void *src, size_t n);
void bmm_dma_sync();
void bmm_flush_cache_range(void *start, size_t size, int direction);

#if defined (__cplusplus)
}
#endif

#endif /* __BMM_LIB_H__ */

