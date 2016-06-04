#ifndef incl_PHP_SIX_H
#define incl_PHP_SIX_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

ZEND_BEGIN_MODULE_GLOBALS(six)
ZEND_END_MODULE_GLOBALS(six)

#if defined(ZTS) && defined(COMPILE_DL_SIX)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

ZEND_EXTERN_MODULE_GLOBALS(six)
#define SIXG(v) ZEND_MODULE_GLOBALS_ACCESSOR(six, v)

#endif
