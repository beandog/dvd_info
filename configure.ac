AC_INIT([dvd_info], [1.3], [https://github.com/beandog/dvd_info/issues], [], [http://dvds.beandog.org/])

dnl This is not a GNU package, so ignore required files / format
AM_INIT_AUTOMAKE([foreign])

dnl Check for C99 support
AC_PROG_CC_C99

dnl Use pkg-config to check for libdvdread, libdvdcss
PKG_CHECK_MODULES([DVDREAD], [dvdread >= 4.2.1])
PKG_CHECK_MODULES([DVDCSS], [libdvdcss >= 1.4.1])

PKG_CHECK_MODULES([MPV], [mpv >= 1.25.0], [has_libmpv=yes])
AC_ARG_ENABLE([player], [AS_HELP_STRING([--enable-player], [build DVD player, requires libmpv])], AS_IF([test "x$has_libmpv" != xyes], AC_MSG_ERROR([DVD player requires libmpv support])))
AC_ARG_ENABLE([ripper], [AS_HELP_STRING([--enable-ripper], [build DVD ripper, requires libmpv, audio and video codecs])], AS_IF([test "x$has_libmpv" != xyes], AC_MSG_ERROR([DVD ripper requires libmpv support])))

dnl check for supported video codecs
AC_ARG_WITH([x264], [AS_HELP_STRING([--with-x264], [with x264 video codec])], [PKG_CHECK_MODULES([X264], [x264], [
	with_video_codecs=yes
	AC_DEFINE(HAVE_X264, [], [libx264])
])])
AC_ARG_WITH([x265], [AS_HELP_STRING([--with-x265], [with x265 video codec])], [	PKG_CHECK_MODULES([X265], [x265], [
	with_video_codecs=yes
	AC_DEFINE(HAVE_X265, [], [libx265])
])])
AC_ARG_WITH([vpx], [AS_HELP_STRING([--with-vpx], [with vpx video codec])], [PKG_CHECK_MODULES([VPX], [vpx], [
	with_video_codecs=yes
	AC_DEFINE(HAVE_VPX, [], [libvpx])
])])
AC_ARG_WITH([theora], [AS_HELP_STRING([--with-theora], [with theora video codec])],[PKG_CHECK_MODULES([THEORA], [theora], [
	with_video_codecs=yes
	AC_DEFINE(HAVE_THEORA, [], [libtheora])
])])

dnl check for supported audio codecs
AC_ARG_WITH([vorbis], [AS_HELP_STRING([--with-vorbis], [with vorbis audio codec])], [PKG_CHECK_MODULES([VORBIS], [vorbis], [
	with_audio_codecs=yes
	AC_DEFINE(HAVE_VORBIS, [], [libvorbis])
])])
AC_ARG_WITH([opus], [AS_HELP_STRING([--with-opus], [with opus video codec])], [PKG_CHECK_MODULES([OPUS], [opus], [
	with_audio_codecs=yes
	AC_DEFINE(HAVE_OPUS, [], [opus])
])])
AC_ARG_WITH([fdk-aac], [AS_HELP_STRING([--with-fdk-aac], [with fdk-aac video codec])], [PKG_CHECK_MODULES([FDK_AAC], [fdk-aac], [
	with_audio_codecs=yes
	AC_DEFINE(HAVE_FDK_AAC, [], [libfdk-aac])
])])

AS_IF([test "x$enable_ripper" = xyes],
	AS_IF([test "x$with_video_codecs" != xyes], AC_MSG_ERROR(Need at least one video and one audio codec enabled for DVD ripper))
	AS_IF([test "x$with_audio_codecs" != xyes], AC_MSG_ERROR(Need at least one video and one audio codec enabled for DVD ripper))
)

dnl The DVD drive tools are OS-specific
AC_CANONICAL_HOST
case "$host_os" in
  linux*)
    linux_drive_tools=yes
    ;;
  netbsd* | openbsd* | freebsd*)
    bsd_drive_status_tool=yes
    bsd_eject_tool=yes
    ;;
  dragonfly*)
    bsd_eject_tool=yes
    ;;
  *)
    dvd_drive_tools=no
    ;;
esac

AM_CONDITIONAL([LINUX_DRIVE_TOOLS], [test "x$linux_drive_tools" = xyes])
AM_CONDITIONAL([BSD_DRIVE_STATUS_TOOL], [test "x$bsd_drive_status_tool" = xyes])
AM_CONDITIONAL([BSD_EJECT_TOOL], [test "x$bsd_eject_tool" = xyes])
AM_CONDITIONAL([DVD_PLAYER], [test "x$enable_player" = xyes])
AM_CONDITIONAL([DVD_RIPPER], [test "x$enable_ripper" = xyes])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
