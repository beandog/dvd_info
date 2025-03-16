all:
	gcc -o dvd_eject dvd_eject.c `pkg-config --libs --cflags libdvdcss`

install:
	sudo cp dvd_eject /usr/local/bin
