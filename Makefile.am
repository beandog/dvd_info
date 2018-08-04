bin_PROGRAMS = dvd_info dvd_copy

if LINUX_DRIVE_TOOLS
bin_PROGRAMS += dvd_drive_status
endif

if BSD_DRIVE_TOOLS
bin_PROGRAMS += dvd_drive_status
endif

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c dvd_ogm.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS) $(DVDCSS_CFLAGS)
dvd_copy_LDADD = $(DVDREAD_LIBS) $(DVDCSS_LIBS)

if LINUX_DRIVE_TOOLS
dvd_drive_status_SOURCES = dvd_drive_status.c
endif

if BSD_DRIVE_TOOLS
dvd_drive_status_SOURCES = bsd_drive_status.c
dvd_drive_status_CFLAGS = $(DVDREAD_CFLAGS)
dvd_drive_status_LDADD = $(DVDREAD_LIBS)
endif
