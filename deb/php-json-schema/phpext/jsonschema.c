/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) Moxie & Laruence                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Moxie<system128@gmail.com> Laruence<laruence@gmail.com>      |
  +----------------------------------------------------------------------+
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_smart_str.h"
#include "php_jsonschema.h"


#ifdef HAVE_JSON
#include "ext/json/php_json.h"
#else
#error("Need json > 1.2.0 ")
#endif

#include "ext/standard/html.h" 

#if HAVE_PCRE || HAVE_BUNDLED_PCRE
#include "ext/pcre/php_pcre.h"
#endif

#include "ext/standard/php_var.h"

#define PHP_JSONSCHEMA_CNAME "JsonSchema"
/* private $errors array */
#define ERRORS_PRO "_errors"
/* private $json mixed */
#define JSON_PRO "_json"
/* private $complexTypes array type schema */
#define COMPLEX_TYPES_PRO "_complexTypes"

/* If you declare any globals in php_jsonschema.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(jsonschema)
 */

/* True global resources - no need for thread safety here */
/* static int le_jsonschema; */

zend_class_entry *jsonschema_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO(jsonschema_construct_args, 0)
ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(add_error_args, 0)
ZEND_ARG_INFO(0, msg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(add_type_args, 0)
ZEND_ARG_INFO(0, type_schema)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(jsonschema_validate_args, 0)
ZEND_ARG_INFO(0, schema)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static zval * get_type(zval * complext_type_schemas, zval * type_name TSRMLS_DC)
 */
static zval * get_type(zval *complext_type_schemas, zval *type_name TSRMLS_DC) {
    zval		*type_schema = NULL;
    zval 		**schema 	 = NULL;
    HashTable 	*complex_type_schemas_table = Z_ARRVAL_P(complext_type_schemas);

	if (zend_hash_find(complex_type_schemas_table, Z_STRVAL_P(type_name), Z_STRLEN_P(type_name), (void **) &schema) == SUCCESS
			&& Z_TYPE_PP(schema) == IS_ARRAY) {
		type_schema = *schema;
	}

    return type_schema;
}
/* }}} */

/** {{{ static void add_error(zval *errors TSRMLS_DC, char * format, ...)
 */
static void add_error(zval *errors TSRMLS_DC, char * format, ...) {
    char 	*msg 	= NULL;
	char 	*html   = NULL;
    uint	msg_len = 0;
	uint	new_len = 0;
    va_list args;

    va_start(args, format);
    msg_len = vspprintf(&msg, 0, format, args);
    va_end(args);

    html = php_escape_html_entities_ex(msg, msg_len, &new_len, 0, ENT_COMPAT, NULL, 1 TSRMLS_CC);

    add_next_index_stringl(errors, html, new_len, 1);
    efree(msg);
}
/* }}} */

/** {{{ static zend_bool check_string(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC)
 */
static zend_bool check_string(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) {
	zval **info = NULL;
	char *str   = NULL;
	uint len    = 0;

    zend_bool isVali = 0;
    do {
		long min_length = 0;
		long max_length = 0;
		char *format	= NULL;

        if (value == NULL || Z_TYPE_P(value) != IS_STRING) {
            convert_to_string(value);
            add_error(errors TSRMLS_CC, "value: '%s' is not a string.", Z_STRVAL_P(value));
            break;
        }
        str = Z_STRVAL_P(value);
        len = Z_STRLEN_P(value);
        if (zend_hash_find(schema_table, ZEND_STRS("min_length"), (void **) &info) == SUCCESS) {
            convert_to_long_ex(info);
			min_length = Z_LVAL_PP(info);
            if (min_length > len) {
                add_error(errors TSRMLS_CC, "value: '%s' is too short.", str);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("max_length"), (void **) &info) == SUCCESS) {
            convert_to_long_ex(info);
            max_length = Z_LVAL_PP(info);
            if (max_length < len) {
                add_error(errors TSRMLS_CC, "value: '%s' is too long.", str);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("format"), (void **) &info) == SUCCESS) {
            convert_to_string_ex(info);
            format = Z_STRVAL_PP(info);

            if (strcmp(format, "date-time") == 0) {
                /**
                 * date-time  This SHOULD be a date in ISO 8601 format of YYYY-MM-
                 * DDThh:mm:ssZ in UTC time.  This is the recommended form of date/
                 * timestamp.
                 */
                break;
            }

            if (strcmp(format, "date") == 0) {
                /**
                 * date  This SHOULD be a date in the format of YYYY-MM-DD.  It is
                 * recommended that you use the "date-time" format instead of "date"
                 * unless you need to transfer only the date part.
                 */
                break;
            }

            if (strcmp(format, "time") == 0) {
                /**
                 * time  This SHOULD be a time in the format of hh:mm:ss.  It is
                 * recommended that you use the "date-time" format instead of "time"
                 * unless you need to transfer only the time part.
                 */
                break;
            }

            if (strcmp(format, "utc-millisec") == 0) {
                /**
                 * utc-millisec  This SHOULD be the difference, measured in
                 * milliseconds, between the specified time and midnight, 00:00 of
                 * January 1, 1970 UTC.  The value SHOULD be a number (integer or
                 * float).
                 */
                break;
            }

            if (strcmp(format, "regex") == 0) {

#if HAVE_PCRE || HAVE_BUNDLED_PCRE

                /**
                 * regex  A regular expression, following the regular expression
                 * specification from ECMA 262/Perl 5.
                 */

                if (zend_hash_find(schema_table, ZEND_STRS("pattern"), (void **) &info) == SUCCESS) {
                    zval *match   = NULL;
                    zval *subpats = NULL;

                    pcre_cache_entry *pce_regexp = NULL;
                    char *pattern = Z_STRVAL_PP(info);

                    MAKE_STD_ZVAL(match);
                    MAKE_STD_ZVAL(subpats);
                    if ((pce_regexp = pcre_get_compiled_regex_cache(ZEND_STRL(pattern) TSRMLS_CC)) == NULL) {
                        break;
                    }

                    php_pcre_match_impl(pce_regexp, str, len, match, subpats /* subpats */
                            , 0/* global */, 0/* ZEND_NUM_ARGS() >= 4 */
                            , 0/*flags PREG_OFFSET_CAPTURE*/, 0/* start_offset */ TSRMLS_CC);

                    if (Z_LVAL_P(match) < 1) {
                        add_error(errors TSRMLS_CC, "'%s' does not match '%s' ", str, pattern);
                        zval_ptr_dtor(&subpats);
                        FREE_ZVAL(match);
                    } else {
                        isVali = 1;
                    }
                } else {
                    add_error(errors TSRMLS_CC, "format-regex: pattern is undefined.");
                }
#endif
                break;
            }

            if (strcmp(format, "color") == 0) {
                /**
                 * color  This is a CSS color (like "#FF0000" or "red"), based on CSS
                 * 2.1 [W3C.CR-CSS21-20070719].
                 */
                break;
            }

            if (strcmp(format, "style") == 0) {
                /**
                 * style  This is a CSS style definition (like "color: red; background-
                 * color:#FFF"), based on CSS 2.1 [W3C.CR-CSS21-20070719].
                 */
                break;
            }

            if (strcmp(format, "phone") == 0) {
#if HAVE_PCRE || HAVE_BUNDLED_PCRE
                /**
                 * phone  This SHOULD be a phone number (format MAY follow E.123).
                 * http://en.wikipedia.org/wiki/E.123
                 */
#endif
                break;
            }

            if (strcmp(format, "uri") == 0) {
                /**
                 * uri  This value SHOULD be a URI..
                 */
                break;
            }

            if (strcmp(format, "email") == 0) {
                /**
                 * email  This SHOULD be an email address.
                 */
                break;
            }

            if (strcmp(format, "ip-address") == 0) {
                /**
                 * ip-address  This SHOULD be an ip version 4 address.
                 */
                break;
            }

            if (strcmp(format, "ipv6") == 0) {
                /**
                 * ipv6  This SHOULD be an ip version 6 address.
                 */
                break;
            }

            if (strcmp(format, "host-name") == 0) {
                /**
                 * host-name  This SHOULD be a host-name.
                 */
                break;
            }

            add_error(errors TSRMLS_CC, "format: '%s' is undefined.", format);
            break;
        }

        isVali = 1;
    } while (0);

    return isVali;
}
/* }}} */

/** {{{ static zend_bool check_number(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC)
 */
static zend_bool check_number(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) {
    zend_bool isVali  = 0;
	zval 	  **info  = NULL;
	double	  num	  = 0;
	double    min_num = 0;
	double    max_num = 0;

    do {
        if (value == NULL || (Z_TYPE_P(value) != IS_LONG && Z_TYPE_P(value) != IS_DOUBLE)) {
            convert_to_string(value);
            add_error(errors TSRMLS_CC, "value: '%s'  is not a number.", Z_STRVAL_P(value));
            break;
        }
        convert_to_double(value);
        num = Z_DVAL_P(value);

        if (zend_hash_find(schema_table, ZEND_STRS("min_num"), (void **) &info) == SUCCESS) {
            convert_to_double_ex(info);
            min_num = Z_DVAL_PP(info);
            if (min_num > num) {
                add_error(errors TSRMLS_CC, "value: %f is less than %f.", num, min_num);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("max_num"), (void **) &info) == SUCCESS) {
            convert_to_double_ex(info);
            max_num = Z_DVAL_PP(info);
            if (max_num < num) {
                add_error(errors TSRMLS_CC, "value: %f is bigger than %f.", num, max_num);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("exclusiveMinimum"), (void **) &info) == SUCCESS) {
			double exclusiveMinimum = 0;
            convert_to_double_ex(info);
            exclusiveMinimum = Z_DVAL_PP(info);
            if (exclusiveMinimum >= num) {
                add_error(errors TSRMLS_CC, "value: %f is less than %f or equal.", num, exclusiveMinimum);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("exclusiveMaximum"), (void **) &info) == SUCCESS) {
			double exclusiveMaximum = 0;
            convert_to_double_ex(info);
            exclusiveMaximum = Z_DVAL_PP(info);
            if (exclusiveMaximum <= num) {
                add_error(errors TSRMLS_CC, "value: %f is bigger than %f or equal.", num, exclusiveMaximum);
                break;
            }
        }

        isVali = 1;

    } while (0);

    return isVali;
}
/* }}} */

/** {{{  static zend_bool check_integer(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) 
 */
static zend_bool check_integer(zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) {
    zend_bool isVali = 0;
    do {
        if (value == NULL || Z_TYPE_P(value) != IS_LONG) {
            convert_to_string(value);
            add_error(errors TSRMLS_CC, "value: '%s' is not a integer.", Z_STRVAL_P(value));
        } else {
            isVali = check_number(errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
        }
    } while (0);

    return isVali;
}
/* }}} */

/** {{{ static zend_bool check_bool(zval *errors, zval *value, HashTable *schema TSRMLS_DC)
 */
static zend_bool check_bool(zval *errors, zval *value, HashTable *schema TSRMLS_DC) {
    zend_bool isVali = 0;

    if (value == NULL || Z_TYPE_P(value) != IS_BOOL) {
        convert_to_string(value);
        add_error(errors TSRMLS_CC, "value: '%s' is not a boolean.", Z_STRVAL_P(value));
    } else {
        isVali = 1;
    }

    return isVali;
}
/* }}} */

/** {{{ static zend_bool check_object(zval *complext_type_schemas, zval *errors, zval *value, HashTable *schema_table TSRMLS_DC)
 */
static zend_bool check_object(zval *complext_type_schemas, zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) {
    zend_bool isVali = 0;
    do {
        if (value == NULL || Z_TYPE_P(value) != IS_OBJECT) {
            convert_to_string(value);
            add_error(errors TSRMLS_CC, "value: '%s' is not an object.", Z_STRVAL_P(value));
            break;
        }
        zval ** schema_properties = NULL;
        if (zend_hash_find(schema_table, ZEND_STRS("properties"), (void **) &schema_properties) == FAILURE) {
            add_error(errors TSRMLS_CC, "properties:schema properties is undefined.");
            break;
        }


        HashTable * schema_properties_table = Z_ARRVAL_PP(schema_properties);
        HashTable * values_prop_table = Z_OBJ_HT_P(value)->get_properties(value TSRMLS_CC);
        zval ** item_schema = NULL;

        HashPosition pointer = NULL;
        zval ** item = NULL;
        isVali = 1;
        char * key;
        uint key_len;
        ulong idx;
        for (zend_hash_internal_pointer_reset_ex(values_prop_table, &pointer);
                zend_hash_get_current_data_ex(values_prop_table, (void **) &item, &pointer) == SUCCESS;
                zend_hash_move_forward_ex(values_prop_table, &pointer)) {

            zend_hash_get_current_key_ex(values_prop_table, &key, &key_len, &idx, 0, &pointer);

            if (zend_hash_find(schema_properties_table, key, key_len, (void **) &item_schema) == FAILURE) {
                add_error(errors TSRMLS_CC, "schema properties: '%s' is undefined.", key);
                break;
            }

            if (!php_jsonschema_check_by_type(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, * item TSRMLS_CC, * item_schema TSRMLS_CC)) {
                isVali = 0;
                break;
            }

        }
        zval ** optional = NULL;
        for (zend_hash_internal_pointer_reset_ex(schema_properties_table, &pointer);
                zend_hash_get_current_data_ex(schema_properties_table, (void **) &item, &pointer) == SUCCESS;
                zend_hash_move_forward_ex(schema_properties_table, &pointer)) {

            zend_hash_get_current_key_ex(schema_properties_table, &key, &key_len, &idx, 0, &pointer);

            if (zend_hash_exists(values_prop_table, key, key_len) == 0) {
                if (zend_hash_find(schema_properties_table, ZEND_STRS("optional"), (void **) &optional) == SUCCESS) {
                    if (Z_BVAL_PP(optional)) {
                        continue;
                    }
                }
                add_error(errors TSRMLS_CC, "value properties: '%s' is not exist, and it's not a optional property.", key);
                break;
            }

        }
    } while (0);
    return isVali;
}
/* }}} */

/** {{{ static zend_bool check_array(zval *complext_type_schemas, zval *errors, zval *value, HashTable *schema_table TSRMLS_DC)
 */
static zend_bool check_array(zval *complext_type_schemas, zval *errors, zval *value, HashTable *schema_table TSRMLS_DC) {
    zend_bool isVali = 0;
    zval 	  **info = NULL;
    zval	  **item = NULL;
    HashPosition pointer = {0}; 

    do {
    	zval **items_schema    = NULL;
        HashTable *value_table = NULL;
		uint size			   = 0;
        if (value == NULL || Z_TYPE_P(value) != IS_ARRAY) {
            convert_to_string(value);
            add_error(errors TSRMLS_CC, "value: '%s' is not an array.", Z_STRVAL_P(value));
            break;
        }

        value_table = Z_ARRVAL_P(value);
        if (zend_hash_find(schema_table, ZEND_STRS("items"), (void **) &items_schema) == FAILURE) {
            add_error(errors TSRMLS_CC, "schema: items schema is undefined.");
            break;
        }

        size = zend_hash_num_elements(value_table);
        if (zend_hash_find(schema_table, ZEND_STRS("minItems"), (void **) &info) == SUCCESS) {
			int min_items = 0;
            convert_to_long_ex(info);
            min_items = Z_LVAL_PP(info);
            if (min_items > size) {
                add_error(errors TSRMLS_CC, "array size: %d is less than %d .", size, min_items);
                break;
            }
        }

        if (zend_hash_find(schema_table, ZEND_STRS("maxItems"), (void **) &info) == SUCCESS) {
			int max_items = 0;
            convert_to_long_ex(info);
            max_items = Z_LVAL_PP(info);
            if (max_items < size) {
                add_error(errors TSRMLS_CC, "array size: %d is bigger than %d .", size, max_items);
                break;
            }
        }

        if (Z_TYPE_PP(items_schema) != IS_ARRAY) {
            break;
        }

        isVali = 1;

        for (zend_hash_internal_pointer_reset_ex(value_table, &pointer);
                zend_hash_get_current_data_ex(value_table, (void **) &item, &pointer) == SUCCESS;
                zend_hash_move_forward_ex(value_table, &pointer)) {
            if (!php_jsonschema_check_by_type(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, * item TSRMLS_CC, *items_schema TSRMLS_CC)) {
                isVali = 0;
                break;
            }

        }
    } while (0);

    return isVali;
}
/* }}} */

/** {{{ PHP_JSONSCHEMA_API zval *php_jsonschema_get_schema(zval *json TSRMLS_DC)
 */
PHP_JSONSCHEMA_API zval *php_jsonschema_get_schema(zval *json TSRMLS_DC) {

    /* $schema */
    zval * schema;
    MAKE_STD_ZVAL(schema);
    array_init(schema);
    SEPARATE_ZVAL_TO_MAKE_IS_REF(&schema);


    switch (Z_TYPE_P(json)) {
        case IS_BOOL:
            add_assoc_string(schema, "type", "boolean", 1);
            add_assoc_bool(schema, "default", 0);
            break;
        case IS_LONG:
            add_assoc_string(schema, "type", "integer", 1);
            add_assoc_long(schema, "default", 0);
            add_assoc_long(schema, "minimum", 0);
            add_assoc_long(schema, "maximum", INT_MAX);
            add_assoc_long(schema, "exclusiveMinimum", 0);
            add_assoc_long(schema, "exclusiveMaximum", INT_MAX);
            break;
        case IS_DOUBLE:
            add_assoc_string(schema, "type", "number", 1);
            add_assoc_long(schema, "default", 0);
            add_assoc_long(schema, "minimum", 0);
            add_assoc_long(schema, "maximum", INT_MAX);
            add_assoc_long(schema, "exclusiveMinimum", 0);
            add_assoc_long(schema, "exclusiveMaximum", INT_MAX);
            break;
        case IS_STRING:
            add_assoc_string(schema, "type", "string", 1);
            add_assoc_string(schema, "format", "regex", 1);
            add_assoc_string(schema, "pattern", "/^[a-z0-9]+$/i", 1);
            add_assoc_string(schema, "type", "string", 1);
            add_assoc_long(schema, "minLength", 0);
            add_assoc_long(schema, "maxLength", INT_MAX);
            break;
        case IS_ARRAY:
            add_assoc_string(schema, "type", "array", 1);
            add_assoc_long(schema, "minItems", 0);
            add_assoc_long(schema, "maxItems", 20);
            HashTable * json_arr_table = Z_ARRVAL_P(json);

            if (zend_hash_num_elements(json_arr_table) > 0) {
                HashPosition pointer = NULL;
                zval ** item = NULL;

                for (zend_hash_internal_pointer_reset_ex(json_arr_table, &pointer);
                        zend_hash_get_current_data_ex(json_arr_table, (void**) &item, &pointer) == SUCCESS;
                        zend_hash_move_forward_ex(json_arr_table, &pointer)) {

                    add_assoc_zval(schema, "items", php_jsonschema_get_schema(*item TSRMLS_CC));
                    break;
                }

            }
            break;
        case IS_OBJECT:
            add_assoc_string(schema, "type", "object", 1);
            HashTable * json_obj_table = Z_OBJPROP_P(json);

            if (zend_hash_num_elements(json_obj_table) > 0) {
                HashPosition pointer = NULL;
                zval ** item = NULL;
                zval * properties;
                MAKE_STD_ZVAL(properties);
                array_init(properties);
                SEPARATE_ZVAL_TO_MAKE_IS_REF(&properties);

                for (zend_hash_internal_pointer_reset_ex(json_obj_table, &pointer);
                        zend_hash_get_current_data_ex(json_obj_table, (void**) &item, &pointer) == SUCCESS;
                        zend_hash_move_forward_ex(json_obj_table, &pointer)) {
                    char * key;
                    uint key_len;
                    ulong idx;
                    zend_hash_get_current_key_ex(json_obj_table, &key, &key_len, &idx, 0, &pointer);
                    add_assoc_zval(properties, key, php_jsonschema_get_schema(*item TSRMLS_CC));

                }
                add_assoc_zval(schema, "properties", properties);
            }
            break;
        case IS_NULL:
            add_assoc_null(schema, "type");
            break;
        default:
            break;
    }
    return schema;
}
/* }}} */

/** {{{ PHP_JSONSCHEMA_API void php_jsonschema_add_type(zval * complext_type_schemas TSRMLS_DC, zval * type_schema TSRMLS_DC) 
 */
PHP_JSONSCHEMA_API void php_jsonschema_add_type(zval * complext_type_schemas TSRMLS_DC, zval * type_schema TSRMLS_DC) {

    HashTable * type_schema_table = Z_ARRVAL_P(type_schema);
    zval ** type_name = NULL;

    if (zend_hash_find(type_schema_table, ZEND_STRS("id"), (void **) &type_name) == SUCCESS) {
        convert_to_string_ex(type_name);

        HashTable * complex_type_schemas_table = Z_ARRVAL_P(complext_type_schemas);
        zend_hash_update(complex_type_schemas_table
                , (* type_name)->value.str.val, (* type_name)->value.str.len, (void *) &type_schema
                , sizeof (zval *), NULL);
    }

}
/* }}} */

/** {{{ PHP_JSONSCHEMA_API zend_bool php_jsonschema_check_by_type(zval *complext_type_schemas, zval *errors, zval *value, zval *schema TSRMLS_DC)
 */
PHP_JSONSCHEMA_API zend_bool php_jsonschema_check_by_type(zval *complext_type_schemas, zval *errors, zval *value, zval *schema TSRMLS_DC) {
    zend_bool isVali = 0;
    HashTable *schema_table = NULL;
    zval 	  **type = NULL;

    do {
        if (schema == NULL || Z_TYPE_P(schema) != IS_ARRAY) {
            break;
        }
        schema_table = Z_ARRVAL_P(schema);
        if (zend_hash_find(schema_table, ZEND_STRS("type"), (void **) &type) == FAILURE) {
            break;
        }

        if (Z_TYPE_PP(type) == IS_ARRAY) {
            HashTable * types_tables = Z_ARRVAL_PP(type);
            HashPosition pointer = NULL;
            zval ** item = NULL;
            zval * tmp = NULL;

            for (zend_hash_internal_pointer_reset_ex(types_tables, &pointer);
                    zend_hash_get_current_data_ex(types_tables, (void **) &item, &pointer) == SUCCESS;
                    zend_hash_move_forward_ex(types_tables, &pointer)) {
                convert_to_string_ex(item);

                zval * tmp_schema = NULL;
                MAKE_STD_ZVAL(tmp_schema);
                array_init(tmp_schema);
                SEPARATE_ZVAL_TO_MAKE_IS_REF(&tmp_schema);

                HashTable * tmp_schema_table = Z_ARRVAL_P(tmp_schema);

                zend_hash_copy(tmp_schema_table, schema_table
                        , (copy_ctor_func_t) zval_add_ref
                        , (void *) &tmp, sizeof (zval *));

                zend_hash_update(tmp_schema_table
                        , "type", sizeof ("type"), (void *) &* item
                        , sizeof (zval *), NULL);

                isVali = php_jsonschema_check_by_type(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, value TSRMLS_CC, tmp_schema TSRMLS_CC);
                FREE_HASHTABLE(tmp_schema_table);
                if (isVali == 1) {
                    break;
                }
            }
            if (types_tables != NULL) {
                FREE_HASHTABLE(types_tables);
            }
            break;
        }
        if (Z_TYPE_PP(type) == IS_STRING) {
            char * type_name = Z_STRVAL_PP(type);
            if (strcmp(type_name, "boolean") == 0) {

                isVali = check_bool(errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else if (strcmp(type_name, "integer") == 0) {

                isVali = check_integer(errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else if (strcmp(type_name, "number") == 0) {

                isVali = check_number(errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else if (strcmp(type_name, "string") == 0) {

                isVali = check_string(errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else
                if (strcmp(type_name, "array") == 0) {

                isVali = check_array(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else if (strcmp(type_name, "object") == 0) {

                isVali = check_object(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, value TSRMLS_CC, schema_table TSRMLS_CC);
            } else if (strcmp(type_name, "null") == 0) {

                isVali = (Z_TYPE_P(value) == IS_NULL) ? 1 : 0;
            } else if (strcmp(type_name, "any") == 0) {

                isVali = 1;
            } else {
                add_error(errors TSRMLS_CC, "type: '%s' is undefined.", type_name);
            }
        }
    } while (0);

    if (zend_hash_find(schema_table, ZEND_STRS("$ref"), (void **) & type) == SUCCESS) {
        zval *type_schema = get_type(complext_type_schemas TSRMLS_CC, *type TSRMLS_CC);
        if (type_schema != NULL) {
            isVali = php_jsonschema_check_by_type(complext_type_schemas TSRMLS_CC
                    , errors TSRMLS_CC, value TSRMLS_CC
                    , type_schema TSRMLS_CC);
        } else {
            add_error(errors TSRMLS_CC, "type schema:'%s' is undefined.", Z_STRVAL_PP(type));
        }

    }

    return isVali;
}
/* }}} */

/** {{{ proto JsonShcema::__construct($json) 
 */
PHP_METHOD(JsonSchema, __construct) {
    char * json_str;
    int json_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s"
            , &json_str, &json_len) == FAILURE) {

        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Expected parameter: $json .");

    }

    zval * self = getThis();

    /* errors array*/
    zval * errors;
    MAKE_STD_ZVAL(errors);
    array_init(errors);
    SEPARATE_ZVAL_TO_MAKE_IS_REF(&errors);
    zend_update_property(Z_OBJCE_P(self), self
            , ZEND_STRL(ERRORS_PRO)
            , errors TSRMLS_DC);

    /* json mixed*/
    zval * json;
    ALLOC_INIT_ZVAL(json);
    SEPARATE_ZVAL_TO_MAKE_IS_REF(&json);
    php_json_decode(json, json_str, json_len, 0 TSRMLS_CC);

    zend_update_property(Z_OBJCE_P(self), self
            , ZEND_STRL(JSON_PRO)
            , json TSRMLS_DC);



    /* complexTypes array type schema */
    zval * complexTypes;
    ALLOC_INIT_ZVAL(complexTypes);
    array_init(complexTypes);
    SEPARATE_ZVAL_TO_MAKE_IS_REF(&complexTypes);
    zend_update_property(Z_OBJCE_P(self), self
            , ZEND_STRL(COMPLEX_TYPES_PRO)
            , complexTypes TSRMLS_DC);


}
/* }}} */

/** {{{ proto JsonShcema::getSchema() 
 */
PHP_METHOD(JsonSchema, getSchema) {
    zval * self = getThis();
    smart_str buf = {0};
    zval * json = zend_read_property(Z_OBJCE_P(self), self
            , ZEND_STRL(JSON_PRO)
            , 1 TSRMLS_CC);

    zval * schema = php_jsonschema_get_schema(json TSRMLS_CC);

    php_json_encode(&buf, schema TSRMLS_DC);

    RETURN_STRINGL(buf.c, buf.len, 1);

    smart_str_free(&buf);
}
/* }}} */

/** {{{ proto JsonShcema::addError($msg)
 */
PHP_METHOD(JsonSchema, addError) {
    char * msg_str;
    int msg_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s"
            , &msg_str, &msg_len) == FAILURE) {

        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Expected parameter: $schema .");

    }
    zval * self = getThis();
    zval * errors = zend_read_property(Z_OBJCE_P(self), self
            , ZEND_STRL(ERRORS_PRO)
            , 1 TSRMLS_CC);
    add_error(errors TSRMLS_CC, msg_str TSRMLS_CC, msg_len TSRMLS_CC);

}
/* }}} */

/** {{{ proto JsonShcema::getErrors()
 */
PHP_METHOD(JsonSchema, getErrors) {

    zval * self = getThis();
    zval * errors = zend_read_property(Z_OBJCE_P(self), self
            , ZEND_STRL(ERRORS_PRO)
            , 1 TSRMLS_CC);
    RETURN_ZVAL(errors, 1, 0);

}
/* }}} */

/** {{{ proto JsonShcema::addType($typeSchema)
 */
PHP_METHOD(JsonSchema, addType) {
    char * type_json;
    int type_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s"
            , &type_json, &type_len) == FAILURE) {

        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Expected parameter: $typeSchema .");

    }

    zval * type_schema;
    ALLOC_INIT_ZVAL(type_schema);
    SEPARATE_ZVAL_TO_MAKE_IS_REF(&type_schema);
    php_json_decode(type_schema, type_json, type_len, 1 TSRMLS_CC);
    if (type_schema == NULL || Z_TYPE_P(type_schema) != IS_ARRAY) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "schema parse error. (PHP 5 >= 5.3.0) see json_last_error(void).");
        return;
    }

    /* $complexTypes */
    zval * self = getThis();
    zval * complext_type_schemas = zend_read_property(Z_OBJCE_P(self), self
            , ZEND_STRL(COMPLEX_TYPES_PRO)
            , 1 TSRMLS_CC);
    php_jsonschema_add_type(complext_type_schemas TSRMLS_CC, type_schema TSRMLS_CC);
}
/* }}} */

/** {{{ proto JsonShcema::validate($schema) 
 */
PHP_METHOD(JsonSchema, validate) {
    char *schema_str = NULL;
    uint  schema_len = 0;
    zval *schema	 = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s"
				,&schema_str, &schema_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	/* schema mixed*/
	MAKE_STD_ZVAL(schema);
	php_json_decode(schema, schema_str, schema_len, 1 TSRMLS_CC);

	if (!schema || Z_TYPE_P(schema) != IS_ARRAY) {
		RETURN_FALSE;
	} else {
		zval *self	 = NULL;
		zval *json 	 = NULL;
		zval *errors = NULL;

		zval *complext_type_schemas = NULL;
		self = getThis();
		json = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL(JSON_PRO), 1 TSRMLS_CC);

		errors = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL(ERRORS_PRO), 1 TSRMLS_CC);

		complext_type_schemas = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL(COMPLEX_TYPES_PRO), 1 TSRMLS_CC);

		RETURN_BOOL(php_jsonschema_check_by_type(complext_type_schemas TSRMLS_CC, errors TSRMLS_CC, json TSRMLS_CC, schema TSRMLS_CC));
	}
}
/* }}} */

/* {{{ jsonschema_functions[]
 *
 * Every user visible function must have an entry in jsonschema_functions[].
 */
zend_function_entry jsonschema_functions[] = {
    PHP_ME(JsonSchema, __construct, jsonschema_construct_args, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(JsonSchema, getSchema, 	NULL, ZEND_ACC_PUBLIC)
    PHP_ME(JsonSchema, addType, 	add_type_args, ZEND_ACC_PUBLIC)
    PHP_ME(JsonSchema, addError, 	add_error_args, ZEND_ACC_PROTECTED)
    PHP_ME(JsonSchema, getErrors, 	NULL, ZEND_ACC_PUBLIC)
    PHP_ME(JsonSchema, validate, 	jsonschema_validate_args, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL
    } /* Must be the last line in jsonschema_functions[] */
};
/* }}} */

/* {{{ jsonschema_module_entry
 */
zend_module_entry jsonschema_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "jsonschema",
    jsonschema_functions,
    PHP_MINIT(jsonschema),
    PHP_MSHUTDOWN(jsonschema),
    PHP_RINIT(jsonschema), /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(jsonschema), /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(jsonschema),
#if ZEND_MODULE_API_NO >= 20010901
    "0.1", /* Replace with version number for your extension */
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_JSONSCHEMA
ZEND_GET_MODULE(jsonschema)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("jsonschema.global_value",      "42", PHP_INI_ALL
                                        , OnUpdateLong, global_value, zend_jsonschema_globals, jsonschema_globals)
    STD_PHP_INI_ENTRY("jsonschema.global_string", "foobar", PHP_INI_ALL
                                        , OnUpdateString, global_string, zend_jsonschema_globals, jsonschema_globals)
PHP_INI_END()
 */
/* }}} */

/* {{{ php_jsonschema_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_jsonschema_init_globals(zend_jsonschema_globals *jsonschema_globals)
{
        jsonschema_globals->global_value = 0;
        jsonschema_globals->global_string = NULL;
}
 */
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(jsonschema) {
    /* If you have INI entries, uncomment these lines
    REGISTER_INI_ENTRIES();
     */
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, PHP_JSONSCHEMA_CNAME, jsonschema_functions);
    jsonschema_ce = zend_register_internal_class(&ce TSRMLS_CC);

    /* errors */
    zend_declare_property_null(jsonschema_ce, ZEND_STRL(ERRORS_PRO), ZEND_ACC_PRIVATE TSRMLS_CC);
    /* json */
    zend_declare_property_null(jsonschema_ce, ZEND_STRL(JSON_PRO), ZEND_ACC_PRIVATE TSRMLS_CC);
    /* complexTypes */
    zend_declare_property_null(jsonschema_ce, ZEND_STRL(COMPLEX_TYPES_PRO), ZEND_ACC_PRIVATE TSRMLS_CC);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(jsonschema) {
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
     */
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(jsonschema) {
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(jsonschema) {
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(jsonschema) {
    php_info_print_table_start();
    php_info_print_table_header(2, "jsonschema support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
     */
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


