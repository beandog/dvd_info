dvd_info
=========

Very small set of C programs for querying DVDs on Linux

* dvd_info - display information about a DVD
* dvd_drive_status - display DVD drive status: opened, closed, waiting, no disc, has disc
* dvd_eject - eject or close a DVD tray with special options
* dvd_copy - copy a single title track to an MPEG file
* dvd_id - return DVD id
* dvd_title - display DVD title
* dvd_xchap - a clone of dvdxchap
* dvd_dump_ifo - dump the IFO files from a DVD (useful for debugging)
* dvd_debug - check a DVD for weird authoring issues

To compile:

autoreconf -fi
./configure
make

Runs on Linux, OpenBSD, NetBSD, and FreeBSD. Have fun. :)

See http://dvds.beandog.org/
