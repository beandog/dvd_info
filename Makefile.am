bin_PROGRAMS = dvd_info dvd_copy dvd_id dvd_title dvd_xchap dvd_debug dvd_cat dvd_av_info dvd_ppm dvd_extract_mpeg2
man_MANS = dvd_info.1

if LINUX_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

if BSD_DRIVE_TOOLS
bin_PROGRAMS += dvd_eject dvd_drive_status
endif

dvd_info_SOURCES = dvd_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_json.c dvd_chapter.c
dvd_info_CFLAGS = $(DVDREAD_CFLAGS)
dvd_info_LDADD = $(DVDREAD_LIBS)

dvd_copy_SOURCES = dvd_copy.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_copy_CFLAGS = $(DVDREAD_CFLAGS)
dvd_copy_LDADD = $(DVDREAD_LIBS) -lrt -lavcodec -lavformat -lavutil -lswscale -lswresample

dvd_cat_SOURCES = dvd_cat.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_cat_CFLAGS = $(DVDREAD_CFLAGS)
dvd_cat_LDADD = $(DVDREAD_LIBS)

dvd_av_info_SOURCES = dvd_av_info.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_av_info_CFLAGS = $(DVDREAD_CFLAGS) -Wno-deprecated-declarations
dvd_av_info_LDADD = $(DVDREAD_LIBS) -lrt -lavcodec -lavformat -lavutil -lswscale -lswresample

dvd_ppm_SOURCES = dvd_ppm.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_ppm_CFLAGS = $(DVDREAD_CFLAGS) -I/usr/include/mpeg2dec
dvd_ppm_LDADD = $(DVDREAD_LIBS) -lmpeg2 -lmpeg2convert

dvd_extract_mpeg2_SOURCES = dvd_extract_mpeg2.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_vts.c dvd_vob.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_extract_mpeg2_CFLAGS = $(DVDREAD_CFLAGS) -I/usr/include/mpeg2dec -Wno-shorten-64-to-32 -Wno-sign-conversion
dvd_extract_mpeg2_LDADD = $(DVDREAD_LIBS) -lmpeg2 -lmpeg2convert 

dvd_id_SOURCES = dvd_id.c dvd_drive.c dvd_vmg_ifo.c dvd_device.c
dvd_id_CFLAGS = $(DVDREAD_CFLAGS)
dvd_id_LDADD = $(DVDREAD_LIBS)

dvd_title_SOURCES = dvd_title.c dvd_drive.c dvd_vmg_ifo.c dvd_device.c
dvd_title_CFLAGS = $(DVDREAD_CFLAGS)
dvd_title_LDADD = $(DVDREAD_LIBS)

dvd_xchap_SOURCES = dvd_xchap.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_time.c dvd_chapter.c
dvd_xchap_CFLAGS = $(DVDREAD_CFLAGS)
dvd_xchap_LDADD = $(DVDREAD_LIBS)

dvd_debug_SOURCES = dvd_debug.c dvd_drive.c dvd_device.c dvd_vmg_ifo.c dvd_track.c dvd_cell.c dvd_video.c dvd_audio.c dvd_subtitles.c dvd_time.c dvd_chapter.c
dvd_debug_CFLAGS = $(DVDREAD_CFLAGS)
dvd_debug_LDADD = $(DVDREAD_LIBS)

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
