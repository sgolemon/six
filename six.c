#include "six.h"

#include "ext/standard/info.h"
#include "zend_smart_str.h"
#include "zend_language_scanner.h"
#include "zend_language_scanner_defs.h"
#include "zend_language_parser.h"

#define zendtext   LANG_SCNG(yy_text)
#define zendleng   LANG_SCNG(yy_leng)
#define zendcursor LANG_SCNG(yy_cursor)
#define zendlimit  LANG_SCNG(yy_limit)

#ifndef MAKE_STD_ZVAL
# define MAKE_STD_ZVAL(pzv) (pzv = emalloc(sizeof(zval)))
#endif

static zend_op_array *(*orig_zend_compile_file)(zend_file_handle *file_handle, int type);
static zend_op_array *(*orig_zend_compile_string)(zval *source_string, char *filename);

typedef struct _six_glyph_pair {
	const char *glyph;
	const char *replacement;
} six_glyph_pair;

static const six_glyph_pair six_glyph_map[] = {
	{ "\xC2\xBC", "(1/4)" }, /* U+00BC VULGAR FRACTION ONE QUARTER */
	{ "\xC2\xBD", "(1/2)" }, /* U+00BD VULGAR FRACTION ONE HALF */
	{ "\xC2\xBE", "(3/4)" }, /* U+00BE VULGAR FRACTION THREE QUARTERS */

	{ "\xE2\x85\x93", "(1/3)" }, /* U+2153 VULGAR FRACTION ONE THIRD */
	{ "\xE2\x86\x92", "->" }, /* U+2192 RIGHTWARDS ARROW */
	{ "\xE2\x87\x92", "=>" }, /* U+21D2 RIGHTWARDS DOUBLE ARROW */
	{ "\xE2\x87\x94", "<=>" }, /* U+21D4 LEFT RIGHT DOUBLE ARROW */
	{ "\xE2\x89\x88", "==" }, /* U+2248 ALMOST EQUAL TO */
	{ "\xE2\x89\xA1", "===" }, /* U+2261 IDENTICAL TO */
	{ "\xE2\x89\xA4", "<=" }, /* U+2264 LESS-THAN OR EQUAL TO */
	{ "\xE2\x89\xA5", ">=" }, /* U+2265 GREATER-THAN OR EQUAL TO */

	{ "\xF0\x9F\x92\xA9", "throw new" }, /* U+1F4A9 PILE OF POO EMOJI */

	{ NULL, NULL }
};

/* {{{ six_rewrite_string - Where most of the magic happens */
/* This is a terrible implementations for perf,
 * but I'm counting on opcache to make that not matter */
static void six_rewrite_string(smart_str *str, const char *text, int leng) {
	const char *text_end = text + leng;

	for (const six_glyph_pair *map = six_glyph_map; (leng >= 1) && map->glyph; ++map) {
		const char *pos;
		int glyph_len = strlen(map->glyph);
		if (glyph_len > leng) continue;
		pos = zend_memnstr(text, map->glyph, glyph_len, text_end);
		if (!pos) continue;
		if (pos > text) {
			six_rewrite_string(str, text, pos - text);
			text = pos;
			leng = text_end - text;
		}
		smart_str_appends(str, map->replacement);
		text += glyph_len;
		leng -= glyph_len;
	}
	if (leng) {
		smart_str_appendl(str, text, leng);
	}
} /* }}} */

/* {{{ six_do_scan */
/* v0 of this ext is going to be dumb and reconstruct an entire string/file in place
 *   to pass to the real parser.
 * v1 can drop that memory wastage by running both scanners in parallel,
 *   one feeding into the other, bouncing the state back and forth.
 */
static void six_do_scan(smart_str *str) {
	zval token, *ret;
	int token_type;

	ZVAL_UNDEF(&token);
	while ((token_type = lex_scan(&token))) {
		if (token_type < 256) {
			smart_str_appendc(str, token_type);
		} else if ((token_type == T_STRING) ||
		           (token_type == T_VARIABLE) ||
		           (token_type == T_STRING_VARNAME)) {
			six_rewrite_string(str, (const char*)zendtext, zendleng);
		} else {
			smart_str_appendl(str, (char*)zendtext, zendleng);
		}
		if (Z_TYPE(token) != IS_UNDEF) {
			zval_dtor(&token);
			ZVAL_UNDEF(&token);
		}
	}
	smart_str_0(str);
} /* }}} */

/* {{{ six_compile_file */
static zend_op_array *six_compile_file(zend_file_handle *file_handle, int type) {
	zend_lex_state original_lex_state;
	smart_str str = { 0 };
	zend_op_array *ret;
	zval zstr;

	zend_save_lexical_state(&original_lex_state);
	if (open_file_for_scanning(file_handle) == FAILURE) {
		zend_restore_lexical_state(&original_lex_state);
		return NULL;
	}

	LANG_SCNG(yy_state) = yycINITIAL;
	smart_str_appends(&str, "?>");
	six_do_scan(&str);
	zend_restore_lexical_state(&original_lex_state);

	ZVAL_STR(&zstr, str.s);
	ret = orig_zend_compile_string(&zstr, (char*)file_handle->filename);
	zval_dtor(&zstr);
	return ret;
} /* }}} */

/* {{{ six_compile_string */
static zend_op_array *six_compile_string(zval *source_string, char *filename) {
	zend_lex_state original_lex_state;
	smart_str str = { 0 };
	zend_op_array *ret;
	zval zstr;

	zend_save_lexical_state(&original_lex_state);
	if (zend_prepare_string_for_scanning(source_string, filename) == FAILURE) {
		zend_restore_lexical_state(&original_lex_state);
		return NULL;
	}

	LANG_SCNG(yy_state) = yycST_IN_SCRIPTING;
	six_do_scan(&str);
	zend_restore_lexical_state(&original_lex_state);

	ZVAL_STR(&zstr, str.s);
	ret = orig_zend_compile_string(&zstr, filename);
	zval_dtor(&zstr);

	return ret;
} /* }}} */

/* {{{ PHP_MINI_FUNCTION */
static PHP_MINIT_FUNCTION(six) {
	orig_zend_compile_file = zend_compile_file;
	zend_compile_file = six_compile_file;

	orig_zend_compile_string = zend_compile_string;
	zend_compile_string = six_compile_string;

	return SUCCESS;
} /* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
static PHP_MSHUTDOWN_FUNCTION(six) {
	zend_compile_file = orig_zend_compile_file;
	zend_compile_string = orig_zend_compile_string;

	return SUCCESS;
} /* }}} */

/* {{{ PHP_MINFO_FUNCTION */
static PHP_MINFO_FUNCTION(six) {
	php_info_print_table_start();
	php_info_print_table_row(2, "six support", "enabled");
	php_info_print_table_end();
} /* }}} */

ZEND_DECLARE_MODULE_GLOBALS(six);

/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(six) {
#if defined(COMPILE_DL_SIX) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
}
/* }}} */

/* {{{ six_module_entry
 */
#ifdef COMPILE_DL_SIX
static
#endif
zend_module_entry six_module_entry = {
	STANDARD_MODULE_HEADER,
	"six",
	NULL, /* functions */
	PHP_MINIT(six),
	PHP_MSHUTDOWN(six),
	NULL, /* RINIT */
	NULL, /* RSHUTDOWN */
	PHP_MINFO(six),
	"7.1.0-dev",
	PHP_MODULE_GLOBALS(six),
	PHP_GINIT(six),
	NULL, /* GSHUTDOWN */
	NULL, /* RPOSTSHUTDOWN */
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

#ifdef COMPILE_DL_SIX
ZEND_GET_MODULE(six)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
