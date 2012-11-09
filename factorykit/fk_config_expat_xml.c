/**
 *   <cases> 
 *	   <case name="hello"/>
 *	   <case name="version"/>
 *   </cases>
 *
 */

#include "fk_config_expat_xml.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <expat.h>

#define LOG_TAG "factorykit"
#include <utils/Log.h>
#include "common.h"

#define TOP_EL "cases"
#define SUB_EL "case" 

typedef struct {
	XML_Parser parser;
	char** name;
	size_t count;
	int in_top_element;
} PrivInfo;

static void start_element(void* userdata, const char* el, const char** attr) 
{
	FkConfig* thiz = (FkConfig*)userdata;
	DECLES_PRIV(priv, thiz);

	if (!strcmp(el, TOP_EL)) {
		priv->in_top_element = 1;
	} else if(priv->in_top_element == 1 && !strcmp(el, SUB_EL)) {
		if(!strcmp("name", attr[0])) {
			priv->count++;
			priv->name = (char**) realloc(priv->name, (priv->count + 1) * sizeof(char*));
			priv->name[priv->count - 1] = strdup(attr[1]);
			priv->name[priv->count] = NULL;
		}
	}
}

static void end_element(void* userdata, const char* el)
{
	FkConfig* thiz = (FkConfig*)userdata;
	DECLES_PRIV(priv, thiz);

	if (!strcmp(el, TOP_EL)) {
		priv->in_top_element = 0;
	}
}

static Ret fk_config_expat_load(FkConfig* thiz, const char* config_file)
{
	DECLES_PRIV(priv, thiz);

	struct stat fstat;
	char* buf;
	int fd;

	if (stat(config_file,  &fstat)) {
		return RET_FAIL;
	}

	buf = (char*)malloc(fstat.st_size);
	if ((fd = open(config_file, O_RDONLY)) < 0) {
		return RET_FAIL;
	}

	read(fd, buf, fstat.st_size);
	close(fd);
	XML_SetElementHandler(priv->parser, start_element, end_element);

	LOGE("%s: buf is %s", __func__, buf);
	LOGE("%s: buf size is %lld", __func__, fstat.st_size);

	if (!XML_Parse(priv->parser, buf, fstat.st_size, 1)) {
		// make log here
		//XML_GetCurrentLineNumber(priv->parser);
		//XML_ErrorString(XML_GetErrorCode(priv->parser));
		LOGE("%s: XML_Parse FAIL at line %d : %s", __func__,
			(int)XML_GetCurrentLineNumber(priv->parser),
			XML_ErrorString(XML_GetErrorCode(priv->parser)));
		return RET_FAIL;
	}

	SAFE_FREE(buf);
	return RET_OK;
}

// bugs here
// Ret fk_config_expat_get_cases_name(FkConfig* thiz, int i, char** name);
// int fk_config_expat_get_cases_name(FkConfig* thiz, int i, char** name);
static Ret fk_config_expat_get_cases_name_by_id(FkConfig* thiz, int i, char** name)
{
	DECLES_PRIV(priv, thiz);

	*name = priv->name[i];
	return RET_OK;
}

static size_t fk_config_expat_get_cases_count(FkConfig* thiz)
{
	DECLES_PRIV(priv, thiz);

	return priv->count;
}

static char** fk_config_expat_get_all_cases(FkConfig* thiz)
{
	DECLES_PRIV(priv, thiz);

	return priv->name;
}

static void fk_config_expat_destroy(FkConfig* thiz)
{
	DECLES_PRIV(priv, thiz);

	XML_ParserFree(priv->parser);
	int i = 0;
	while (priv->name[i] != NULL) {
		free(priv->name[i]);
		priv->name[i] = NULL;
		i++;
	}

	SAFE_FREE(priv->name);
	SAFE_FREE(thiz);
}

FkConfig* fk_config_expat_create()
{
	FkConfig* thiz = (FkConfig*) malloc(sizeof(FkConfig) + sizeof(PrivInfo));

	if (thiz != NULL) {
		DECLES_PRIV(priv, thiz);

		thiz->load = fk_config_expat_load;
		thiz->get_cases_name_by_id = fk_config_expat_get_cases_name_by_id;
		thiz->get_cases_count = fk_config_expat_get_cases_count;
		thiz->get_all_cases = fk_config_expat_get_all_cases;

		thiz->destroy = fk_config_expat_destroy;

		priv->parser = XML_ParserCreate(NULL);
		XML_SetUserData(priv->parser, (void*)thiz);
		priv->count = 0;
		priv->name = NULL;
		priv->in_top_element = 0;
	}

	return thiz;
}

#ifdef FK_CONFIG_EXPAT_XML_TEST

#include <stdio.h>
#include <assert.h>

#define XML_CONFIG_FILE "./test-cases.xml"

int main(int argc, char* argv[])
{
	int i = 0;
	size_t count = 0;
	char** tests;

	FkConfig* config = fk_config_expat_create();

	assert(RET_OK == fk_config_load(config, XML_CONFIG_FILE));
	count = fk_config_get_cases_count(config);
	tests = fk_config_get_all_cases(config);

	printf("cases count is %d\n", count);

	i = 0;
	while (tests[i] != NULL) {
		printf("cases %d: %s\n", i, tests[i]);
		i++;
	}

	printf("=================\n");
	for (i = 0; i < count; i++) {
		char* name;
		assert(RET_OK == fk_config_get_cases_name_by_id(config, i, &name));
		printf("test[%d] -- %s\n", i, name);
	}


	fk_config_destroy(config);

	return 0;
}

#endif // FK_CONFIG_EXPAT_XML_TEST
