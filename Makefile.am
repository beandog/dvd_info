bin_PROGRAMS = dvd_info dvd_copy dvd_id dvd_title
man_MANS = dvd_info.1

if LINUX_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

if BSD_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_xchap.c dvd_debug.c dvd_chapter.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS)
dvd_copy_LDADD = $(DVDREAD_LIBS)

dvd_title_SOURCES = dvd_title.c dvd_drive.c dvd_vmg_ifo.c dvd_device.c
dvd_title_CFLAGS = $(DVDREAD_CFLAGS)
dvd_title_LDADD = $(DVDREAD_LIBS)

dvd_id_SOURCES = dvd_id.c dvd_drive.c dvd_vmg_ifo.c dvd_device.c
dvd_id_CFLAGS = $(DVDREAD_CFLAGS)
dvd_id_LDADD = $(DVDREAD_LIBS)

if LINUX_DRIVE_TOOLS
dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = $(DVDCSS_CFLAGS)
dvd_eject_LDADD = $(DVDCSS_LIBS)
dvd_drive_status_SOURCES = dvd_drive_status.c
endif

if BSD_DRIVE_TOOLS
dvd_eject_SOURCES = bsd_eject.c
dvd_drive_status_SOURCES = bsd_drive_status.c
dvd_drive_status_CFLAGS = $(DVDREAD_CFLAGS)
dvd_drive_status_LDADD = $(DVDREAD_LIBS)
endif

install-exec-hook:
	$(LN_S) -f $(DESTDIR)$(bindir)/dvd_info $(DESTDIR)$(bindir)/dvd_xchap

uninstall:
	-rm $(DESTDIR)$(bindir)/dvd_info
	-rm $(DESTDIR)$(bindir)/dvd_copy
	-rm $(DESTDIR)$(bindir)/dvd_xchap
if LINUX_DRIVE_TOOLS
	-rm $(DESTDIR)$(bindir)/dvd_eject
	-rm $(DESTDIR)$(bindir)/dvd_drive_status
endif
if BSD_DRIVE_TOOLS
	-rm $(DESTDIR)$(bindir)/dvd_eject
	-rm $(DESTDIR)$(bindir)/dvd_drive_status
endif
