/*
 * INTEL CONFIDENTIAL
 * Copyright 2005-2006 Intel Corporation All Rights Reserved.
 * 
 * The source code contained or described herein and all documents
 * related to the source code (Material) are owned by Intel Corporation
 * or its suppliers or licensors.  Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material contains
 * trade secrets and proprietary and confidential information of Intel
 * or its suppliers and licensors. The Material is protected by worldwide
 * copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded,
 * posted, transmitted, distributed, or disclosed in any way without Intel's
 * prior express written permission.
 * 
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.

 *(C) Copyright 2006 Marvell International Ltd.  
 * All Rights Reserved 
 */

#ifndef _M2D_H
#define _M2D_H

#define MAX_DEVICE_GMEM_SIZE		(8*1024*1024)
#define MAX_CONTEXT_GMEM_SIZE		(4*1024*1024)

#define GCU_RINGBUF_SIZE		(16384)
#define GCU_SCRATCHREG_NR		(8)

#include <asm/ioctl.h>

#define M2DIO_SUBMIT		_IOW('2',  1, struct iovec *)
#define M2DIO_SYNC		_IOW('2',  2, int)

#define M2DIO_REQUEST_MEM	_IOW('2', 10, struct m2d_mem_request *)
#define M2DIO_RELEASE_MEM	_IOW('2', 11, unsigned long)
#define M2DIO_FLUSH_MEM		_IOW('2', 12, unsigned long)

#define M2DIO_GET_BUS_ADDR	_IOW('2', 20, unsigned long)

//#define M2DIO_QUERY_GCU		_IOR('2', 20, struct m2d_gcu_stat *)

#define M2D_GRAPHICS_MEM	0
#define M2D_FRAME_BUFFER	1
#define M2D_REGISTERS		2
#define M2D_RING_BUFFER		3

#define M2D_ATTR_COHERENT	0x00
#define M2D_ATTR_WRITECOMBINE	0x10
#define M2D_ATTR_CACHEABLE	0x20

#define M2D_MEM_REQ_TYPE(f)		(f & 0x0f)
#define M2D_MEM_REQ_ATTR(f)		(f & 0xf0)

struct m2d_mem_req {
	unsigned int	req_type;
	unsigned int	req_size;
	unsigned long	phys_addr;
	unsigned long	mmap_addr;
	unsigned long	mmap_size;
};

#define M2D_SUBMIT_MODE_NDELAY	(1 << 0)
#define M2D_SUBMIT_MODE_SYNC	(1 << 1)

struct m2d_submit_req {
	unsigned int	mode;
	void *		base;
	size_t		len;
};

#endif
