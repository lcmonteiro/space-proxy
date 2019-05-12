/* 
 * File:   CTunnelSP.cpp
 * Author: root
 * 
 * Created on December 19, 2014, 12:07 PM
 */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <cstdio>
#include <cstring>

#include "CTunnelSP.h"

/**
 * 
 */
static int SpeedToBaudrate(int speed) {
	switch (speed) {
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		case 230400: return B230400;
		default: return B9600;
	}
	return B9600;
}

/**
 * 
 */
CTunnelSP::CTunnelSP(const char* dev, int speed) : CTunnelBase(dev) {
	_speed = speed;
}

CTunnelSP::~CTunnelSP() {
}

/**
 * 
 */
bool CTunnelSP::Open() {
	/**
	 * Open port
	 */
	if ((_fd = open(_dev.data(), O_RDWR | O_NOCTTY)) < 0) {
		ERROR( << "open():" << strerror(errno));
		return false;
	}
	/**
	 * Configure
	 */
	struct termios tty;

	std::memset(&tty, 0, sizeof (tty));
	/**
	 * Read configuration
	 */
	if (tcgetattr(_fd, &tty) != 0) {
		ERROR( << "tcgetattr():" << strerror(errno));
		return false;
	}
	/**
	 * Set baud rate
	 */
	cfsetospeed(&tty, _speed);
	cfsetispeed(&tty, _speed);
	/**
	 *  Other Port Stuff 
	 */
	tty.c_cflag = 0;
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag |= SpeedToBaudrate(_speed);
	tty.c_cflag |= CS8;
	tty.c_iflag = IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	tcflush(_fd, TCIFLUSH);
	/**
	 * Write configuration
	 */
	if (tcsetattr(_fd, TCSANOW, &tty) != 0) {
		ERROR( << "tcsetattr():" << strerror(errno));
		return false;
	}
	INFO( << "Open port: " << _dev << " baudrate: " << _speed);
	return true;
}
