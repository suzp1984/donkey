#ifndef FK_SQLITE_H
#define FK_SQLITE_H

#define FACTORY_DATABASE "/productinfo/factory.db"

#define FK_STR2INT_TABLE "str2int"
#define FK_STR2STR_TABLE "str2str"

#define FK_SQL_LEN 128
#define FK_SQLSTR2INT_ERR 0x7FFFFFFF
#define FK_SQLSTR2STR_ERR "NO VALUE"

typedef struct _str2int_sqlresult_t {
	const char* name;
	int result;
} str2int_sqlresult_t;

int fk_sqlite_create(void);
int fk_sql_str2int_set(const char* name, int value);
int fk_sql_str2int_get(const char* name);

#endif // FK_SQLITE_H
