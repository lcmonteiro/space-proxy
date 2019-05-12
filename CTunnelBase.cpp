/* 
 * File:   CTunnelBase.cpp
 * Author: root
 * 
 * Created on December 19, 2014, 10:18 AM
 */
#include <netinet/in.h>

#include <unistd.h>
#include <errno.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "CTunnelBase.h"
#include "CTunnelTCP.h"
/**
 * 
 */
static inline int read_fd(int fd, unsigned char* buf, int size, struct timeval* time);

static inline int write_fd(int fd, unsigned char* buf, int size, struct timeval* time);

/**
 * 
 */
fd_set CTunnelBase::_rd_set;

#ifdef __DEBUG__
bool CTunnelBase::_info = true;

bool CTunnelBase::_debug = true;

bool CTunnelBase::_trace = true;
#else
bool CTunnelBase::_info = true;

bool CTunnelBase::_debug = false;

bool CTunnelBase::_trace = false;
#endif

ofstream CTunnelBase::_null("/dev/null");

/**
 * 
 */

CTunnelBase::CTunnelBase(const char* dev) {
	_dev = dev;
	_fd = -1;
}

CTunnelBase::~CTunnelBase() {
}

bool CTunnelBase::Open() {
	_fd = dup(STDOUT_FILENO);
	return true;
}

bool CTunnelBase::Close() {
	close(_fd);
	return true;
}

/**
 */
CPacket& CTunnelBase::operator>>(CPacket &p) {
	int n = 0;

	if (EventRX()) {
		if (Read((unsigned char*) &n, sizeof (n), 10) != sizeof(n)) {
			throw string("read: ") + strerror(errno);
		}
		p.resize(ntohs(n));
		/**
		 * 
		 */
		if (Read(p.data(), p.size(), 10) != (int) p.size()) {
			throw string("Read: ") + strerror(errno);
		}
		DEBUG( << "CTunnelBase::read: " << _fd << " " << p.size());
	}
	return p;
}

CPacket& CTunnelBase::operator<<(CPacket &p) {
	int n = 0;

	if (p.size()) {

		DEBUG( << "CTunnelBase::write: " << _fd << " " << p.size());

		n = htons(p.size());

		if (Write((unsigned char*) &n, sizeof (n), 10) < 0) {
			throw string("write: ") + strerror(errno);
		}
		/**
		 * 
		 */
		if (Write(p.data(), p.size(), 10) != (int) p.size()) {
			throw string("Write: ") + strerror(errno);
		}
		p.resize(0);
	}
	return p;
}

int CTunnelBase::Read(unsigned char* buf, int size, int timeout_sec, int timeout_ms) {
	struct timeval time = {
		timeout_sec,
		timeout_ms * 1000,
	};
	return read_fd(_fd, buf, size, &time);
}

int CTunnelBase::Write(unsigned char* buf, int size, int timeout_sec, int timeout_ms) {
	struct timeval time = {
		timeout_sec,
		timeout_ms * 1000,
	};
	return write_fd(_fd, buf, size, &time);
}

bool CTunnelBase::Wait(int timeout, CTunnelBase* pTunn, ...) {
	va_list va;
	fd_set err_set;
	/**
	 * 
	 */
	int max_fd = 0;

	FD_ZERO(&_rd_set);
	FD_ZERO(&err_set);
	va_start(va, pTunn);

	FD_SET(STDIN_FILENO, &_rd_set);
	for (; pTunn; pTunn = va_arg(va, CTunnelBase*)) {
		if (pTunn->_fd > 0) {
			FD_SET(pTunn->_fd, &_rd_set);
			FD_SET(pTunn->_fd, &err_set);

			if (max_fd < pTunn->_fd) {
				max_fd = pTunn->_fd;
			}
		}
	}
	va_end(va);
	/**
	 * 
	 */
	struct timeval time = {
		timeout,
		0
	};
	if (select(max_fd + 1, &_rd_set, NULL, &err_set, &time) < 0) {
		throw string("select: ") + strerror(errno);
	}
	/**
	 * check error
	 */
	va_start(va, pTunn);

	for (; pTunn; pTunn = va_arg(va, CTunnelBase*)) {
		if (FD_ISSET(pTunn->_fd, &err_set)) {
			va_end(va);
			return false;
		}
	}
	va_end(va);

	return true;
}

void CTunnelBase::Process() {

	if (Event(STDIN_FILENO)) {
		char op = 0;

		std::cin.get(op);

		switch (op) {
			case 'i':
			{
				INFO( << "Disable Information MSGs");
				_info = false;
				break;
			}
			case 'I':
			{
				INFO( << "Enable Information MSGs");
				_info = true;
				break;
			}
			case 't':
			{
				INFO( << "Disable Trace MSGs");
				_trace = false;
				break;
			}
			case 'T':
			{
				INFO( << "Enable Trace MSGs");
				_trace = true;
				break;
			}
			case 'd':
			{
				INFO( << "Disable Debug MSGs");
				_debug = false;
				break;
			}
			case 'D':
			{
				INFO( << "Enable Debug MSGs");
				_debug = true;
				break;
			}
			default:
			{
				DEBUG( << "wrong Option");
			}
		}
	}
}

const std::string CTunnelBase::DateTime() {
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof (buf), "%Y-%m-%d %X", &tstruct);
	return buf;
}

/**
 * 
 */
static inline int sel_read_fd(int fd, struct timeval* time) {
	register int nfds = 0;
	fd_set r_fds, e_fds;

	FD_ZERO(&r_fds);
	FD_ZERO(&e_fds);

	while ((time->tv_usec > 0) || (time->tv_sec > 0)) {

		FD_SET(fd, &r_fds);
		FD_SET(fd, &e_fds);

		nfds = select(fd + 1, &r_fds, NULL, &e_fds, time);

		if (nfds < 0) {
			return -1;
		}
		if ((nfds > 0) && FD_ISSET(fd, &e_fds)) {
			return -1;
		}
		if ((nfds > 0) && FD_ISSET(fd, &r_fds)) {
			return 1;
		}
	}
	return 0;
}

static inline int read_fd(int fd, unsigned char* buf, int size, struct timeval* time) {
	register int ret = -1;
	register int sz = size;

	while (sz) {
		if ((ret = sel_read_fd(fd, time)) <= 0) {
			if (ret == 0) {
				break;
			}
			return ret;
		}
		if ((ret = read(fd, buf, sz)) <= 0) {
			return ret;
		}
		buf += ret;
		sz -= ret;
	}
	return size - sz;
}

static inline int sel_write_fd(int fd, struct timeval* time) {
	register int nfds = 0;
	fd_set w_fds, e_fds;

	FD_ZERO(&w_fds);
	FD_ZERO(&e_fds);

	while ((time->tv_usec > 0) || (time->tv_sec > 0)) {

		FD_SET(fd, &w_fds);
		FD_SET(fd, &e_fds);

		nfds = select(fd + 1, NULL, &w_fds, &e_fds, time);

		if (nfds < 0) {
			return -1;
		}
		if ((nfds > 0) && FD_ISSET(fd, &e_fds)) {
			return -1;
		}
		if ((nfds > 0) && FD_ISSET(fd, &w_fds)) {
			return 1;
		}
	}
	return 0;
}

static inline int write_fd(int fd, unsigned char* buf, int size, struct timeval* time) {
	register int ret = -1;
	register int sz = size;

	while (sz) {
		if ((ret = sel_write_fd(fd, time)) <= 0) {
			if (ret == 0) {
				break;
			}
			return ret;
		}
		if ((ret = write(fd, buf, sz)) <= 0) {
			return ret;
		}
		buf += ret;
		sz -= ret;
	}
	return size - sz;
}
