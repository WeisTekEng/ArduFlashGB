AC_INIT([gbshooper], [0.1], [zako@ladecadence.net])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_CONFIG_HEADERS([config.h])
AC_CHECK_LIB(pthread, pthread_create,,
	[AC_MSG_ERROR([required library pthread missing])])
PKG_CHECK_MODULES([libusb], [libusb >= 0.1.12])
PKG_CHECK_MODULES([libftdi], [libftdi >= 0.19])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
