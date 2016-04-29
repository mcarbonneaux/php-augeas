/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Pedro Padron <ppadron@php.net>                               |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_exceptions.h"
#include "ext/standard/info.h"
#include "augeas.h"
#include "php_augeas.h"
#include "ext/dom/xml_common.h"
#include "ext/simplexml/php_simplexml.h"
#include "ext/simplexml/php_simplexml_exports.h"

/* {{{ zend_class_entry */
zend_class_entry *augeas_ce_Augeas;
zend_class_entry *augeas_ce_AugeasException;
/* }}} */

/* {{{ ZEND_BEGIN_ARG_INFO */
ZEND_BEGIN_ARG_INFO_EX(arginfo_Augeas__construct, 0, 0, 0)
	ZEND_ARG_INFO(0, root)
	ZEND_ARG_INFO(0, loadpath)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_get, 0)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_dump_to_xml, 0)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_match, 0)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_set, 0)
	ZEND_ARG_INFO(0, path)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_save, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_rm, 0)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_Augeas_mv, 0)
	ZEND_ARG_INFO(0, source)
	ZEND_ARG_INFO(0, destination)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_Augeas_insert, 0, 0, 2)
	ZEND_ARG_INFO(0, path)
	ZEND_ARG_INFO(0, label)
	ZEND_ARG_INFO(0, order)
ZEND_END_ARG_INFO();
/* }}} */

/* {{{ augeas_methods */
static zend_function_entry augeas_methods[] = {
	PHP_ME(Augeas, __construct, arginfo_Augeas__construct, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, get, arginfo_Augeas_get, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, dump_to_xml, arginfo_Augeas_dump_to_xml, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, set, arginfo_Augeas_set, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, match, arginfo_Augeas_match, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, rm, arginfo_Augeas_rm, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, save, arginfo_Augeas_save, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, mv, arginfo_Augeas_mv, ZEND_ACC_PUBLIC)
	PHP_ME(Augeas, insert, arginfo_Augeas_insert, ZEND_ACC_PUBLIC)
	{ NULL, NULL, NULL }
};
/* }}} */ 

/* {{{ augeas_module_entry
 */
zend_module_entry augeas_module_entry = {
	STANDARD_MODULE_HEADER,
	"augeas",
	NULL,
	PHP_MINIT(augeas),
	PHP_MSHUTDOWN(augeas),
	NULL,
	NULL,
	PHP_MINFO(augeas),
	PHP_AUGEAS_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_AUGEAS
ZEND_GET_MODULE(augeas)
#endif

/* {{{ AUGEAS_FROM_OBJECT */
#define AUGEAS_FROM_OBJECT(intern, object) \
    { \
        php_augeas_object *obj = (php_augeas_object*) zend_object_store_get_object(object TSRMLS_CC); \
        intern = obj->augeas; \
        if (!intern) { \
			zend_throw_exception(augeas_ce_AugeasException, "Invalid or unitialized Augeas object", 0 TSRMLS_CC); \
            RETURN_FALSE; \
        } \
    }
/* }}} */

/* {{{ augeas_object_dtor() */
static void augeas_object_dtor(void *object, zend_object_handle handle TSRMLS_DC)
{
	php_augeas_object *intern;

	intern = (php_augeas_object *) object;

	zend_hash_destroy(intern->zo.properties);
	FREE_HASHTABLE(intern->zo.properties);

	if (intern->augeas) {
		aug_close(intern->augeas);
	}	

	efree(object);
}
/* }}} */

/* {{{ augeas_object_new() */
static zend_object_value augeas_object_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	php_augeas_object *intern;
	zval *tmp;

	intern = emalloc(sizeof(php_augeas_object));
	memset(intern, 0, sizeof(php_augeas_object));
	intern->zo.ce = class_type;

	ALLOC_HASHTABLE(intern->zo.properties);
	zend_hash_init(intern->zo.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->zo.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#else
       object_properties_init((zend_object*) &(intern->zo), class_type);
#endif

	retval.handle = zend_objects_store_put(intern, augeas_object_dtor, NULL, NULL TSRMLS_CC);
	retval.handlers = zend_get_std_object_handlers();

	return retval;
	
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(augeas)
{
	zend_class_entry ce, ce_exception;
	zend_class_entry *default_exception=NULL;

	/* Register AugeasException class (inherits Exception) */
	INIT_CLASS_ENTRY(ce_exception, "AugeasException", NULL);
	#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	        default_exception= zend_exception_get_default();
	#else
	        default_exception= zend_exception_get_default(TSRMLS_C);
	#endif
	augeas_ce_AugeasException = zend_register_internal_class_ex(&ce_exception, default_exception, NULL TSRMLS_CC); 

	/* Register Augeas class */
	INIT_CLASS_ENTRY(ce, "Augeas", augeas_methods);
	augeas_ce_Augeas = zend_register_internal_class(&ce TSRMLS_CC);
	augeas_ce_Augeas->create_object = augeas_object_new;

	/* Register Augeas class constants */
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_NONE", sizeof("AUGEAS_NONE")-1, AUG_NONE TSRMLS_CC);
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_SAVE_BACKUP", sizeof("AUGEAS_SAVE_BACKUP")-1, AUG_SAVE_BACKUP TSRMLS_CC);
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_SAVE_NEWFILE", sizeof("AUGEAS_SAVE_NEWFILE")-1, AUG_SAVE_NEWFILE TSRMLS_CC);
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_TYPE_CHECK", sizeof("AUGEAS_TYPE_CHECK")-1, AUG_TYPE_CHECK TSRMLS_CC);
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_NO_STDINC", sizeof("AUGEAS_NO_STDINC")-1, AUG_NO_STDINC TSRMLS_CC);

	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_INSERT_BEFORE", sizeof("AUGEAS_INSERT_BEFORE")-1, 1 TSRMLS_CC);
	zend_declare_class_constant_long(augeas_ce_Augeas, "AUGEAS_INSERT_AFTER", sizeof("AUGEAS_INSERT_AFTER")-1, 0 TSRMLS_CC);
 
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(augeas)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(augeas)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "augeas support", "enabled");
	php_info_print_table_row(2, "augeas version", PHP_AUGEAS_VERSION);
	php_info_print_table_end();

}
/* }}} */

/* {{{ proto void Augeas::__construct([string $root[, string $loadpath[, int $flags]]]) */
PHP_METHOD(Augeas, __construct)
{
	char *root = "/";
	char *loadpath = "";
	int root_len, loadpath_len, resource_id;
	long flags = AUG_NONE;
	php_augeas_object *obj;
	zval *resource;
	zval *this;


	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s!s!l", &root, &root_len, &loadpath, &loadpath_len, &flags) == FAILURE) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(root TSRMLS_CC) != 0) {
		RETURN_FALSE;
	}

	obj = (php_augeas_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	obj->augeas = aug_init(root, loadpath, flags);

	if (!obj->augeas) zend_throw_exception(augeas_ce_AugeasException, "could not initialize augeas resource", 0 TSRMLS_CC);

}
/* }}} */

/* {{{ proto string Augeas::get(string $path) */
PHP_METHOD(Augeas, get)
{
	char *path, *value;
	char **matches;
	int path_len, retval;
	php_augeas_object *obj;
	augeas *aug_intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (path_len < 1) {
		RETURN_FALSE;
	}   

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_get(aug_intern, path, (const char **) &value);

	switch (retval) {

		/* No match */
		case 0:
			RETURN_NULL();
			break;

		/* Exactly one match */
		case 1:
			/* If the specified path is a tree, NULL is returned */
			if (value == NULL) {
				RETURN_NULL();
			}
			RETURN_STRING(value, 1);
			break;

		default:
			zend_throw_exception(augeas_ce_AugeasException, "specified path is invalid", 0 TSRMLS_CC);
			break;
	}
	
}
/* }}} */

/* {{{ proto string Augeas::dump_to_xml(string $path);
       dump to xml string. */
PHP_METHOD(Augeas, dump_to_xml)
{
	char *path, *value;
	char **matches;
	int path_len, retval;
	php_augeas_object *obj;
	augeas *aug_intern;
	xmlNodePtr xmldoc=NULL;
        xmlBufferPtr buffer=NULL;
	int size = 0;
	zend_bool xmlFlag = 1;
	zend_class_entry **ce;
	php_sxe_object *sxe;
	int ret;

	// return simple element object
	php_libxml_node_object *interndoc;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &path, &path_len, &xmlFlag) == FAILURE) {
		RETURN_FALSE;
	}

	if (path_len < 1) {
		RETURN_FALSE;
	}   

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_to_xml(aug_intern, path, &xmldoc, 0);
	switch (retval) {

		/* export success */
		case 0:
			if (! xmldoc) {
				RETURN_NULL();
			}

			if (xmlFlag) {
			   /*
			   zend_lookup_class("SimpleXMLElement",16,&ce TSRMLS_CC);
			   object_init_ex(return_value, *ce);

			   //sxe = sxe_object_new(*ce TSRMLS_CC);
			   sxe=zend_object_store_get_object_by_handle(return_value->value.obj.handle TSRMLS_CC);
			   //sxe->document = return_value->document;
			   php_libxml_increment_doc_ref((php_libxml_node_object *)sxe, xmldoc->doc TSRMLS_CC);
			   php_libxml_increment_node_ptr((php_libxml_node_object *)sxe, xmldoc, NULL TSRMLS_CC);

			   //return_value->type = IS_OBJECT;
			   //return_value->value.obj = php_sxe_register_object(sxe TSRMLS_CC);
			   */

			   //zend_lookup_class("DOMDocument",11,&ce TSRMLS_CC);
			   //object_init_ex(return_value, *ce);

			   /*
			   sxe = (php_sxe_object *)zend_objects_get_address(return_value TSRMLS_CC);
			   //sxe->document=xmldoc->doc;
			   php_libxml_increment_doc_ref(sxe, xmldoc->doc TSRMLS_CC);
			   php_libxml_increment_node_ptr(sxe, xmldoc, (void *)interndoc TSRMLS_CC);
			   */
			   xmlDocPtr xdoc=xmlNewDoc(NULL);
			   xmlDocSetRootElement(xdoc,xmldoc);
			   xmldoc->doc=xdoc;
			   DOM_RET_OBJ( xmldoc, &ret, NULL);
			   //interndoc = (php_libxml_node_object *)zend_objects_get_address(return_value TSRMLS_CC);
			   //php_libxml_increment_doc_ref(interndoc, xdoc TSRMLS_CC);
			   //php_libxml_increment_node_ptr(interndoc, xmldoc, (void *)interndoc TSRMLS_CC);

			} else {
			  // dump xmldoc to xmlbuffer
			  buffer = xmlBufferCreate();
			  size = xmlNodeDump(buffer, xmldoc->doc, xmldoc, 0, 1);
			  if (size==-1) {
				  zend_throw_exception(augeas_ce_AugeasException, "xmlNodeDump failed", 0 TSRMLS_CC);
				  break;
			  }

			  // duplicate xml string emallocated memory
			  value=emalloc(size);
			  memcpy(value,buffer->content,size);
			  value[size]='\0';

			  xmlFreeNode(xmldoc);
			  xmlBufferFree(buffer);

			  RETURN_STRING(value, 0);
			}
			break;

		default:
			zend_throw_exception(augeas_ce_AugeasException, "XML export failed", 0 TSRMLS_CC);
			break;
	}

	
}
/* }}} */

/* {{{ proto boolean Augeas::set(string $path, string $value);
	   Sets the value of $path to $value */
PHP_METHOD(Augeas, set)
{
	int path_len, value_len, retval;
	char *path, *value;
	augeas *aug_intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &value, &value_len)) {
		RETURN_FALSE;
	}

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_set(aug_intern, path, value);

	if (retval == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

/* {{{  proto array Augeas::match(string $path);
		Returns an array with all the matches */
PHP_METHOD(Augeas, match)
{
	int i;
	char *path;
	char *value, *tmpval;
	char **matches;
	augeas *aug_intern;

	int path_len, retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
		RETURN_FALSE;
	}

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_match(aug_intern, path, &matches);

	array_init(return_value);

	if (retval == -1) {
		zend_throw_exception(augeas_ce_AugeasException, "the specified path is invalid", 0 TSRMLS_CC);
		return;
	}
	
	if (retval > 0) {
		for (i=0; i<retval; i++) {

			aug_get(aug_intern, matches[i], (const char **) &tmpval);

			if (tmpval) {
				add_assoc_string(return_value, matches[i], tmpval, 1);
			} else {
				add_assoc_null(return_value, matches[i]);
			}
		}
	}

	/* if no matches were found, an empty array (return_value) is returned */

}
/* }}} */

/* {{{ proto boolean Augeas::rm(string $path);
	   Removes a node and all it's children */
PHP_METHOD(Augeas, rm)
{   
	int path_len, value_len, retval;
	char *path, *value;
	augeas *aug_intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len, &value, &value_len)) {
		RETURN_FALSE;
	}

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	/* aug_rm returns the number of removed entries */
	retval = aug_rm(aug_intern, path);

	RETURN_LONG(retval);
}
/* }}} * /

/* {{{ proto boolean Augeas::mv(string $source, string $destination);
       Moves $source node to $destination. If $destination exists, it will be overwritten. */
PHP_METHOD(Augeas, mv)
{
	char *src, *dst;
	int src_len, dst_len, retval;
	augeas *aug_intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &src, &src_len, &dst, &dst_len)) {
		RETURN_FALSE;
	}

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_mv(aug_intern, src, dst);

	if (retval == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

/*  {{{ proto boolean Augeas::insert(string $path, string $label[, int $order]);
		Inserts a new sibling of path expression $path with label $label before or after $path, depending on $order. $path must match exactly one node in the tree. */
PHP_METHOD(Augeas, insert)
{	
	char *path, *label;
	int retval, path_len, label_len;
	int order = 0;
	augeas *aug_intern;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|l", &path, &path_len,
		&label, &label_len, &order)) {
		RETURN_FALSE;
	}

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_insert(aug_intern, path, label, order);

	if (retval == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{  proto boolean Augeas::save();
		Saves the parts of the tree that have been changed into their respective files. */
PHP_METHOD(Augeas, save)
{
	zval *zaug;
	int retval;
	augeas *aug_intern;

    if (ZEND_NUM_ARGS()) {
        WRONG_PARAM_COUNT;
    }

	AUGEAS_FROM_OBJECT(aug_intern, getThis());

	retval = aug_save(aug_intern);

	if (retval == 0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}

}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
