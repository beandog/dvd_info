# dvd_info

The dvd_info project is a set of utilities for accessing DVDs:

* `dvd_info` - display information about a DVD in human-readable or JSON output; display track chapters for Matroska container format
* `dvd_copy` - copy a DVD track plus chapters to the filesystem or stdout
* `dvd_drive_status` - display drive status: open, closed, closed with disc, or polling (Linux only)
* `dvd_backup` - back up an entire DVD to the filesystem
* `dvd_player` - a small DVD player using libmpv as backend
* `dvd_rip` - a shell script to rip DVDs using ffmpeg-7.0 and above
* `dvd_debug` - display debugging information about a DVD provided by libdvdread
* `dvd_trip` - a small DVD ripper using libmpv as backend - supports x264, x265, vp8, vp9, aac, opus, mp4, mkv, and webm
* `dvd_debug` - display debugging information about a DVD provided by libdvdread

# Requirements

* libdvdread >= 6.1.0
* libdvdcss for decryption
* libmpv for `dvd_rip` and `dvd_player`
* ffmpeg 7.0 or greater for dvd_rip

# Installation

For ``dvd_info``, ``dvd_copy``, ``dvd_drive_status``, and ``dvd_debug`` only:

```
cmake .
make
sudo make install
```

Building ``dvd_player`` and ``dvd_rip`` requires ``mpv`` to be installed with libmpv support first.

# Documentation

Run ``--help`` for each program to see its options.

There are man pages for each program as well.

# Development

The git repo on github is intended to build from a straight pull, if there are any issues please file a bug. Releases are provided at milestones.

# Usage

A DVD source can be a device name, a directory, or an ISO.

If you're going to do a lot of reads on a disc drive, you can mount it so the filesystem access can be cached -- this is especially helpful when using the programs frequently on the same DVD.

```
sudo mount /dev/sr0 -o ro -t udf /mnt
```

# Compatability

I try porting my code to other systems, and in addition to Linux distros, it runs fine on BSD systems as well. It also builds with gcc and clang.

# See also

  * <https://dvds.beandog.org> - my site all about DVDs
  * <https://github.com/beandog/bluray_info> - similar tools for Blu-ray discs

# Support

Please file any issues at https://github.com/beandog/dvd_info/issues or email me at ``steve.dibb@gmail.com``

# Copyright

Licensed under GPL-2. See https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
