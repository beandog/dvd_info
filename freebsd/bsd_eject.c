#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/cdio.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ucred.h>

#define DEFAULT_DVD_DEVICE "/dev/cd0"

/**
 *      _          _          _           _
 *   __| |_   ____| |    ___ (_) ___  ___| |_
 *  / _` \ \ / / _` |   / _ \| |/ _ \/ __| __|
 * | (_| |\ V / (_| |  |  __/| |  __/ (__| |_
 *  \__,_| \_/ \__,_|___\___|/ |\___|\___|\__|
 *                 |_____| |__/
 *
 * A simple eject utility for FreeBSD. I wrote this because the OS doesn't
 * install an eject program by default, and the one in the package tree you
 * have to specify the drive.
 *
 * This will use the default device, unmount it, and eject it. You can pass
 * an option to force an eject as well. If unmount doesn't work, it will try
 * and continue anyway.
 *
 * FreeBSD does have a native way to eject drives:
 * $ cdcontrol cd0 eject
 * And close the tray:
 * $ cdcontrol cd0 close
 *
 * You can see which drives you have:
 * $ camcontrol devlist
 * $ camcontrol identify cd0
 *
 * Have fun!
 *
 */

void print_usage(void);

int main(int argc, char **argv) {

	int opt;
	bool eject_device = true;
	bool close_tray = false;
	bool force_unmount = false;
	bool verbose = false;
	bool debug = false;

	bool exit_program = false;

	// with getopt, options have to be passed before device 
	while ((opt = getopt(argc, argv, "fhtvz")) != -1) {

		switch(opt) {
			case 'f':
				force_unmount = true;
				break;
			case 't':
				close_tray = true;
				eject_device = false;
				break;
			case 'v':
				verbose = true;
				break;
			case 'z':
				verbose = true;
				debug = true;
				break;
			case 'h':
			case '?':
				print_usage();
				exit_program = true;
				break;
			default:
				break;
		}

	}

	if(exit_program)
		return 0;

	char device_filename[PATH_MAX];
	memset(device_filename, '\0', PATH_MAX);

	if(argv[optind]) {
		strncpy(device_filename, argv[optind], PATH_MAX - 1);
	} else {
		strncpy(device_filename, DEFAULT_DVD_DEVICE, PATH_MAX - 1);
	}

	int retval;
	struct stat device_stat;

	retval = stat(device_filename, &device_stat);
	if(retval) {
		fprintf(stderr, "Could not open device %s\n", device_filename);
		return 1;
	}

	int fd;
	fd = open(device_filename, O_RDONLY | O_NONBLOCK);

	if(fd < 0) {
		fprintf(stderr, "Could not open device %s\n", device_filename);
		return 1;
	}

	int num_mounts;
	struct statfs *mount_buffer;
	num_mounts = getmntinfo(&mount_buffer, MNT_NOWAIT);

	int x;
	bool device_mounted = false;
	char mount_point[PATH_MAX] = {'\0'};
	for(x = 0; x < num_mounts; x++) {

		if(strncmp("/dev/", mount_buffer[x].f_mntfromname, 5))
			continue;

		if(debug)
			printf("Device %s mounted at %s\n", mount_buffer[x].f_mntfromname, mount_buffer[x].f_mntonname);

		if(strncmp(device_filename, mount_buffer[x].f_mntfromname, strlen(device_filename)) == 0) {
			device_mounted = true;

			strncpy(mount_point, mount_buffer[x].f_mntonname, PATH_MAX - 1);

			if(verbose)
				fprintf(stderr, "Found mounted device %s at %s\n", device_filename, mount_point);

		}

	}

	int umount_flags = 0;
	if(force_unmount)
		umount_flags = MNT_FORCE;

	if(device_mounted) {

		retval = unmount(mount_point, umount_flags);
		
		if(retval) {
			fprintf(stderr, "Failed to unmount %s from %s\n", device_filename, mount_point);
		}

		if(verbose)
			fprintf(stderr, "Successfully unmounted %s from %s\n", device_filename, mount_point);
		
	}

	// This may fail, but the device can still be accessed
	// cdcontrol.c does the same thing, it doesn't check for a return value
	// https://cgit.freebsd.org/src/plain/usr.sbin/cdcontrol/cdcontrol.c
	retval = ioctl(fd, CDIOCALLOW);

        if(eject_device) {
		if(verbose)
			fprintf(stderr, "Ejecting %s\n", device_filename);
		retval = ioctl(fd, CDIOCEJECT);
	} else if(close_tray) {
		if(verbose)
			fprintf(stderr, "Closing tray for %s\n", device_filename);
		retval = ioctl(fd, CDIOCCLOSE);
	}
	
	if(retval < 0) {
		if(eject_device)
			fprintf(stderr, "Failed to eject %s\n", device_filename);
		else if(close_tray)
			fprintf(stderr, "Failed to close tray for %s\n", device_filename);
		return 1;
	}

	close(fd);

        return 0;

}

void print_usage(void) {

	printf("dvd_eject [options] [device]\n");
	printf("  -h     Display help output\n");
	printf("  -f     Force unmount\n");
	printf("  -t     Close tray instead of opening\n");
	printf("  -v     Display verbose output\n");
	printf("\n");
	printf("Default device is %s\n", DEFAULT_DVD_DEVICE);

}
