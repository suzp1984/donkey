#ifndef _HW_TYPEDEF_H
#define _HW_TYPEDEF_H

typedef enum _RET 
{
	RET_OK,
	RET_OOM,
	RET_STOP,
	RET_INVALID_PARAMS,
	RET_FAIL
} RET;

#define return_if_fail(p) if(!(p)) \
	    {printf("%s:%d Warning: "#p" failed.\n", \
	    		        __func__, __LINE__); return;}
#define return_val_if_fail(p, ret) if(!(p)) \
	    {printf("%s:%d Warning: "#p" failed.\n",\
	    		    __func__, __LINE__); return (ret);}

#endif //_HW_TYPEDEF_H
