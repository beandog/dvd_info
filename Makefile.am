bin_PROGRAMS = dvd_info dvd_copy

if DVD_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

dvd_info_SOURCES = dvd_info.c dvd_init.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_xchap.c dvd_debug.c dvd_chapter.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_init.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS) -Weverything -Wno-padded -Wno-reserved-id-macro -Wno-documentation -Wno-documentation-unknown-command -Wno-disabled-macro-expansion -Wno-missing-braces -Wno-unreachable-code
dvd_copy_LDADD = $(DVDREAD_LIBS)

dvd_eject_SOURCES = dvd_eject.c
dvd_eject_CFLAGS = $(DVDCSS_CFLAGS)
dvd_eject_LDADD = $(DVDCSS_LIBS)

dvd_drive_status_SOURCES = dvd_drive_status.c

install-exec-hook:
	$(LN_S) -f $(DESTDIR)$(bindir)/dvd_info $(DESTDIR)$(bindir)/dvd_xchap
	$(LN_S) -f $(DESTDIR)$(bindir)/dvd_info $(DESTDIR)$(bindir)/dvd_id
	$(LN_S) -f $(DESTDIR)$(bindir)/dvd_info $(DESTDIR)$(bindir)/dvd_title
	$(LN_S) -f $(DESTDIR)$(bindir)/dvd_info $(DESTDIR)$(bindir)/dvd_json
