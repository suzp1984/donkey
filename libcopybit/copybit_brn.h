/***********************************
 * add by broncho
 ***********************************/
#ifndef __COPYBIT_BRN__
#define __COPYBIT_BRN__

#ifdef __cplusplus
extern "C"{
#endif

#define ERROR_UNSUPPORT_FORMAT 1

struct android_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct android_img {
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t offset;
	void * base;
	int memory_id;
};

struct android_blit_req {
	struct android_img src;
	struct android_img dst;
	struct android_rect src_rect;
	struct android_rect dst_rect;
	uint32_t alpha;
	uint32_t transp_mask;
	uint32_t flags;
};


#ifdef __cplusplus
}
#endif

#endif /*COPYBIT_BRN_H*/
