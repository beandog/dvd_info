AC_INIT([dvd_info], [1.0], [steve.dibb@gmail.com], [https://github.com/beandog/dvd_info])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC_C99

PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.2.13])

AC_ARG_ENABLE([json], [  --enable-json	Enable JSON output format],
	[case "${enableval}" in
		yes)
			json=true
			PKG_CHECK_MODULES([JANSSON], [jansson >= 2.3.1])
			;;
		no)
			json=false
			;;
		*)
			AC_MSG_ERROR([bad value ${enableval} for --enable-json])
			;;
	esac],
[json=false])
AM_CONDITIONAL([JSON], [test x$json = xtrue])

dnl The dvd_drive_status and dvd_eject programs rely on Linux kernel internals,
dnl so they are ignored on other operating systems.
AC_CANONICAL_HOST
case "$host_os" in
  linux*)
    dvd_drive_tools=yes
    ;;
  *)
    dvd_drive_tools=no
    ;;
esac
AM_CONDITIONAL([DVD_DRIVE_TOOLS], [test x$dvd_drive_tools = xyes])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
