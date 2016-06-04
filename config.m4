dnl $Id$
dnl config.m4 for extension six

PHP_ARG_ENABLE(six, whether to enable PHP 6 support,
[  --disable-six          Disable PHP 6 support], yes)

if test "$PHP_SIX" != "no"; then
  PHP_NEW_EXTENSION(six, six.c, $ext_shared)
fi
