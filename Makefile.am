bin_PROGRAMS = dvd_info

if DVD_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_time.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS)

if JSON
dvd_info_CFLAGS += $(JANSSON_CFLAGS)
dvd_info_LDADD += $(JANSSON_LIBS)
endif

dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = $(DVDCSS_CFLAGS)
dvd_eject_LDADD = $(DVDCSS_LIBS)

dvd_drive_status_SOURCES = dvd_drive_status.c
