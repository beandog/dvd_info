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
#else
#define DEFAULT_DVD_DEVICE "/dev/dvd"
#endif

#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

bool dvd_device_access(const char *device_filename);

int dvd_device_open(const char *device_filename);

int dvd_device_close(const int dvd_fd);

bool dvd_device_is_hardware(const char *device_filename);

bool dvd_device_is_image(const char *device_filename);

#endif
