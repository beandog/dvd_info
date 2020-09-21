#ifndef DVD_INFO_DEVICE_H
#define DVD_INFO_DEVICE_H

// Default DVD device
#if defined (__linux__)
#include <linux/cdrom.h>
#define DEFAULT_DVD_DEVICE "/dev/sr0"
#elif defined (__DragonFly__)
#define DEFAULT_DVD_DEVICE "/dev/cd0"
#elif defined (__FreeBSD__)
#define DEFAULT_DVD_DEVICE "/dev/cd0"
#elif defined (__NetBSD__)
#define DEFAULT_DVD_DEVICE "/dev/cd0d"
#elif defined (__OpenBSD__)
#define DEFAULT_DVD_DEVICE "/dev/rcd0c"
#elif defined (__APPLE__) && defined (__MACH__)
#define DEFAULT_DVD_DEVICE "/dev/disk1"
#elif defined (__CYGWIN__)
#define DEFAULT_DVD_DEVICE "D:\\"
#elif defined(_WIN32)
#define DEFAULT_DVD_DEVICE "/d/"
#else
#define DEFAULT_DVD_DEVICE "/dev/dvd"
#endif

#endif
