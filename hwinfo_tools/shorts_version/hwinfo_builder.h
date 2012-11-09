#ifndef H_HWINFO_BUILDER
#define H_HWINFO_BUILDER

struct _HwinfoBuilder;
typedef struct _HwinfoBuilder HwinfoBuilder;

int hwinfo_config_parser(int fd);
int hwinfo_prepare();
int hwinfo_build_buffer();

#endif
