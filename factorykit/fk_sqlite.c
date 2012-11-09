#include "fk_sqlite.h"
#include "sqlite3.h"
#include <string.h>
#include <stdlib.h>

#define LOG_TAG "fk_sqlite"
#include <utils/Log.h>

static int fk_query_success;

static int fk_str2int_callback(void* param, int argc, char** argv, char** cname)
{
	int i;
	str2int_sqlresult_t* sqlresult = (str2int_sqlresult_t*)param;
	for (i = 0; i < argc; i++) {
		if (!strcmp("value", cname[i])) {
			fk_query_success = 1;
			sqlresult->result = atoi(argv[i]);
		}
	}

	return 0;
}

int fk_sqlite_create(void)
{
	sqlite3* db = NULL;
	char* errmsg = NULL;
	char sql_createtable[FK_SQL_LEN];
	int rc = 0;
	int ret = 0;

	rc = sqlite3_open(FACTORY_DATABASE, &db);

	if (rc != 0) {
		LOGE("%s: open %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
				sqlite3_errcode(db), sqlite3_errmsg(db));
		ret = -1;
		goto out;
	} else {
		LOGE("%s: open %s success", __func__, FACTORY_DATABASE);
	}

	memset(sql_createtable, 0, FK_SQL_LEN);
	sprintf(sql_createtable, "CREATE TABLE %s(name VARCHAR(32) PRIMARY KEY,value INTEGER);", FK_STR2INT_TABLE);

	rc = sqlite3_exec(db, sql_createtable, NULL, NULL, &errmsg);
	
	if (rc == 1) {
		LOGE("%s: %s already exists\n", __func__, FK_STR2INT_TABLE);
	} else if (rc != 0) {
		LOGE("%s: create table fail, errmsg = %s [%d:%s]", __func__, errmsg, 
				sqlite3_errcode(db), sqlite3_errmsg(db));
		ret = -1;
		goto out;
	} else {
		LOGE("%s: create table %s success", __func__, FK_STR2INT_TABLE);
	}
	
out:
	sqlite3_close(db);
	return ret;
}

int fk_sql_str2int_set(const char* name, int value)
{
	sqlite3* db = NULL;
	char sqlbuf[FK_SQL_LEN];
	char* errmsg = NULL;
	int rownum;
	int colnum;
	char** result;
	int rc = 0;
	int ret = 0;

	rc = sqlite3_open(FACTORY_DATABASE, &db);
	if (rc != 0) {
		LOGE("%s: open %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
				sqlite3_errcode(db), sqlite3_errmsg(db));
		ret = -1;
		goto out;
	} else {
		LOGE("%s: open %s success", __func__, FACTORY_DATABASE);
	}

	memset(sqlbuf, 0, FK_SQL_LEN);
	sprintf(sqlbuf, "SELECT * FROM %s WHERE name='%s'", FK_STR2INT_TABLE, name);
	rc = sqlite3_get_table(db, sqlbuf, &result, &rownum, &colnum, &errmsg);
	sqlite3_free_table(result);

	if (rownum > 0) {
		memset(sqlbuf, 0, FK_SQL_LEN);
		sprintf(sqlbuf, "UPDATE %s SET value=%d where name='%s';", FK_STR2INT_TABLE, value, name);

		rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
		if (rc != 0) {
			LOGE("%s: update table %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
					sqlite3_errcode(db), sqlite3_errmsg(db));
			ret = -1;
			goto out;
		}
	} else {
		memset(sqlbuf, 0, FK_SQL_LEN);
		sprintf(sqlbuf, "INSERT INTO %s VALUES('%s',%d);", FK_STR2INT_TABLE, name, value);

		rc = sqlite3_exec(db, sqlbuf, NULL, NULL, &errmsg);
		if (rc != 0) {
			LOGE("%s: insert table %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
					sqlite3_errcode(db), sqlite3_errmsg(db));
			ret = -1;
			goto out;
		}
	}

out:
	sqlite3_close(db);
	return ret;
}

int fk_sql_str2int_get(const char* name)
{
	sqlite3* db = NULL;
	char* errmsg = NULL;
	char sqlbuf[FK_SQL_LEN];

	str2int_sqlresult_t sqlresult;
	int rc = 0;
	int ret = -1;
	
	memset(&sqlresult, 0, sizeof(sqlresult));
	sqlresult.name = name;
	sqlresult.result = 0;

	rc = sqlite3_open(FACTORY_DATABASE, &db);
	if (rc != 0) {
		LOGE("%s: open %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
				sqlite3_errcode(db), sqlite3_errmsg(db));
		ret = -1;
		goto out;
	} else {
		LOGE("%s: open %s success", __func__, FACTORY_DATABASE);
	}

	memset(sqlbuf, 0, FK_SQL_LEN);
	sprintf(sqlbuf, "SELECT * FROM %s WHERE name='%s'", FK_STR2INT_TABLE, name);

	fk_query_success = 0;
	rc = sqlite3_exec(db, sqlbuf, &fk_str2int_callback, &sqlresult, &errmsg);
	if (rc != 0) {
		LOGE("%s: query table %s fail [%d:%s]\n", __func__, FACTORY_DATABASE,
				sqlite3_errcode(db), sqlite3_errmsg(db));
		ret = -1;
		goto out;
	}

	if (fk_query_success == 1) {
		ret = sqlresult.result;
	} else {
		ret = -1;
	}
out:
	sqlite3_close(db);
	return ret;
}
