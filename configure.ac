AC_INIT([dvd_info], [1.0], [steve.dibb@gmail.com], [https://github.com/beandog/dvd_info])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC_C99

PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.2.13])
PKG_CHECK_MODULES([JANSSON], [jansson >= 2.3.1])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
