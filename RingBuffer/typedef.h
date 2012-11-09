#ifndef TYPEDEF_H
#define TYPEDEF_H

#ifdef __cplusplus
#define DECLS_BEGIN extern "C" {
#define DECLS_END }
#else
#define DECLS_BEGIN
#define DECLS_END
#endif/*__cplusplus*/

typedef enum _Ret {
	RET_OK,
	RET_ERROR,
	RET_FAIL,
}Ret;

#define return_if_fail(p) if(!(p)) { \
	printf("%s:%d Warning: "#p" failed. \n", \
			__func__, __LINE__); return;}

#define return_val_if_fail(p, ret) if(!(p)) { \
	printf("%s:%d Warning: "#p" failed. \n", \
			__func__, __LINE__); return(ret);}


#endif
