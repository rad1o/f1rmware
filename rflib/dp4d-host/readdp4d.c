#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <err.h>
#include <linux/serial.h>
#include <sys/time.h>
#include <sys/types.h>

#include "../dp4d.h"

static int rate_to_constant(int baudrate) {
#define B(x) case x: return B##x
	switch(baudrate) {
		B(50);     B(75);     B(110);    B(134);    B(150);
		B(200);    B(300);    B(600);    B(1200);   B(1800);
		B(2400);   B(4800);   B(9600);   B(19200);  B(38400);
		B(57600);  B(115200); B(230400); B(460800); B(500000); 
		B(576000); B(921600); B(1000000);B(1152000);B(1500000); 
        B(4000000);
	default: return 0;
	}
#undef B
}    

/* Open serial port in raw mode, with custom baudrate if necessary */
int serial_open(const char *device, int rate)
{
	struct termios options;
	struct serial_struct serinfo;
	int fd;
	int speed = 0;

	/* Open and configure serial port */
	if ((fd = open(device,O_RDWR|O_NOCTTY)) == -1)
		return -1;

	speed = rate_to_constant(rate);

	if (speed == 0) {
		/* Custom divisor */
		serinfo.reserved_char[0] = 0;
		if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0)
			return -1;
		serinfo.flags &= ~ASYNC_SPD_MASK;
		serinfo.flags |= ASYNC_SPD_CUST;
		serinfo.custom_divisor = (serinfo.baud_base + (rate / 2)) / rate;
        fprintf(stderr, "setting custom divisor %d for baud_base %d\n", serinfo.custom_divisor, serinfo.baud_base);
		if (serinfo.custom_divisor < 1) 
			serinfo.custom_divisor = 1;
		if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0)
			return -1;
		if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0)
			return -1;
		if (serinfo.custom_divisor * rate != serinfo.baud_base) {
			warnx("actual baudrate is %d / %d = %f",
			      serinfo.baud_base, serinfo.custom_divisor,
			      (float)serinfo.baud_base / serinfo.custom_divisor);
		}
	}

	fcntl(fd, F_SETFL, 0);
	tcgetattr(fd, &options);
	cfsetispeed(&options, speed ?: B38400);
	cfsetospeed(&options, speed ?: B38400);
	cfmakeraw(&options);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~CRTSCTS;
	if (tcsetattr(fd, TCSANOW, &options) != 0)
		return -1;

	return fd;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "call this like\n%s <ttydev>\n", argv[0]);
        return -1;
    }
    int fd = serial_open(argv[1], 4000000);
    if(fd == -1) {
        fprintf(stderr, "error opening serial device, exiting.\n");
        return -1;
    }

    write(fd, "GO", 2);

    ssize_t r;
    dp4d_pkg pkg;
    while(1) {
        r = read(fd, (void*)&pkg, sizeof(pkg));
        if(r == -1) {
            perror(NULL);
            return -1;
        }
        if(r == sizeof(pkg)) {
            if(pkg.version == 0) {
                fprintf(stdout, "Data: ID=%08x x=%d y=%d btn=%d\n", pkg.id, pkg.x, pkg.y, pkg.button);
            } else if(pkg.version == 1) {
                fprintf(stdout, "New:  ID=%08x x=%d y=%d btn=%d\n", pkg.id, pkg.x, pkg.y, pkg.button);
            } else if(pkg.version == 2) {
                fprintf(stdout, "Gone: ID=%08x\n", pkg.id);
            }
        }
    }
}
