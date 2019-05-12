/* 
 * File:   CTunnelI.cpp
 * Author: root
 * 
 * Created on December 11, 2014, 11:08 AM
 */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <string>

#include "CTunnelI.h"

CTunnelI::CTunnelI(const char* dev, const char* local) : CTunnelBase(dev) {
	_local_addr = local;
}

CTunnelI::~CTunnelI() {
}

bool CTunnelI::Open() {

	if (!Create()) {
		return false;
	}
	if (!Enable()) {
		INFO( << "Enable failed");
	}
	return true;
}

bool CTunnelI::Create() {
	struct ifreq ifr;
	/*
	 *  open the clone device 
	 */
	if ((_fd = open("/dev/net/tun", O_RDWR)) < 0) {
		if (mkdir("/dev/net", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
			ERROR( << "mkdir (/dev/net):" << strerror(errno));
			return false;
		}
		if (mknod("/dev/net/tun", S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH, makedev(10, 200)) < 0) {
			ERROR( << "mknod(/dev/net/tun):" << strerror(errno));
			return false;
		}
		if ((_fd = open("/dev/net/tun", O_RDWR)) < 0) {
			ERROR( << "Opening (/dev/net/tun):" << strerror(errno));
			return false;
		}
	}
	std::memset(&ifr, 0, sizeof (ifr));

	if (_dev.empty()) {
		ERROR( << "dev name is empty" << std::endl);
		return false;
	}
	std::strncpy(ifr.ifr_name, _dev.c_str(), IFNAMSIZ);
	/*
	 *  try to create the device 
	 */
#ifdef __INFO_PACKET__
	ifr.ifr_flags = IFF_TUN;
#else
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
#endif
	if (ioctl(_fd, TUNSETIFF, (void *) &ifr) < 0) {
		ERROR( << "ioctl(TUNSETIFF):" << strerror(errno));
		close(_fd);
		return false;
	}
	return true;
}

bool CTunnelI::Enable() {

	struct ifreq ifr;

	/**
	 * 
	 */
	std::memset(&ifr, 0, sizeof (ifr));

	std::strncpy(ifr.ifr_name, _dev.c_str(), IFNAMSIZ);

	/**
	 * Enable
	 */
	int s = -1;
	int d = AF_INET6;
	size_t prefix_pos;
	unsigned char addr_buf[16];

	prefix_pos = _local_addr.find("/");

	std::string address = (prefix_pos == std::string::npos) ? _local_addr : _local_addr.substr(0, prefix_pos);

	if (inet_pton(d, address.c_str(), addr_buf) <= 0) {
		d = AF_INET;
		if (inet_pton(d, address.c_str(), addr_buf) <= 0) {
			ERROR( << "Invalid IP address.");
			return false;
		}
	}
	if ((s = socket(d, SOCK_DGRAM, 0)) < 0) {
		ERROR( << "socket(AF_INET6)" << strerror(errno));
		return false;
	}
	/**
	 * Turn on interface
	 */
	if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
		ERROR( << "ioctl(SIOCGIFFLAGS): " << strerror(errno));
		return false;
	}
	ifr.ifr_flags |= IFF_UP;
	if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0) {
		ERROR( << "ioctl(SIOCGIFFLAGS): " << strerror(errno));
		return false;
	}
	/**
	 * Set address
	 */
	switch (d) {
		case AF_INET6:
		{
			std::string prefix = (prefix_pos == std::string::npos) ? "64" : _local_addr.substr(prefix_pos + 1);

			struct in6_ifreq {
				struct in6_addr addr;
				uint32_t prefixlen;
				unsigned int ifindex;
			} ifr6;

			if (ioctl(s, SIOGIFINDEX, &ifr) < 0) {
				ERROR( << "ioctl(SIOCGIFFLAGS): " << strerror(errno));
				return false;
			}
			std::memcpy(&ifr6.addr, addr_buf, sizeof (ifr6.addr));
			ifr6.ifindex = ifr.ifr_ifindex;
			ifr6.prefixlen = atoi(prefix.c_str());

			if (ioctl(s, SIOCSIFADDR, &ifr6) < 0) {
				ERROR( << "ioctl(SIOCSIFADDR): " << ifr.ifr_name << " : " << strerror(errno));
				return false;
			}
			break;
		}
		case AF_INET:
		{
			struct sockaddr_in addr;

			/**
			 *  ip  
			 */
			std::memset(&addr, 0, sizeof (addr));
			std::memcpy(&addr.sin_addr.s_addr, addr_buf, sizeof (addr.sin_addr.s_addr));
			addr.sin_family = AF_INET;

			memcpy(&ifr.ifr_addr, &addr, sizeof (struct sockaddr));

			if (ioctl(s, SIOCSIFADDR, &ifr) < 0) {
				ERROR( << "ioctl(SIOCSIFADDR): " << ifr.ifr_name << " : " << strerror(errno));
				return false;
			}
			/**
			 * netmask
			 */
			std::string prefix = (prefix_pos == std::string::npos) ? "24" : _local_addr.substr(prefix_pos + 1);

			unsigned int keepbits;
			buffer_t mask(4);

			keepbits = atoi(prefix.c_str());
			mask.Integer(keepbits > 0 ? 0x00 - (1 << (32 - keepbits)) : 0xFFFFFFFF);

			std::memset(&addr, 0, sizeof (addr));
			std::memcpy(&addr.sin_addr.s_addr, mask.data(), sizeof (addr.sin_addr.s_addr));
			addr.sin_family = AF_INET;

			memcpy(&ifr.ifr_addr, &addr, sizeof (struct sockaddr));

			if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0) {
				ERROR( << "ioctl(SIOCSIFADDR): " << ifr.ifr_name << " : " << strerror(errno));
				return false;
			}
			break;
		}
	}
	close(s);

	return true;
}

bool CTunnelI::Close() {
	if (ioctl(_fd, TUNSETPERSIST, 0) < 0) {
		ERROR( << "disabling TUNSETPERSIST:" << strerror(errno));
		return false;
	}
	close(_fd);

	return true;
}

/**
 */
CPacket& CTunnelI::operator>>(CPacket &p) {
	int n = 0;

	if (EventRX()) {
		p.resize(p.capacity());

		if ((n = read(_fd, p.data(), p.capacity())) <= 0) {
			throw std::string("read: ") + strerror(errno);
		} else {
			p.resize(n);
		}
		DEBUG( << "CTunnelI::read: " << _fd << " " << p.size());
	}
	return p;
}

CPacket& CTunnelI::operator<<(CPacket &p) {
	int n = 0;

	if (p.size()) {

		DEBUG( << "CTunnelI::write: " << _fd << " " << p.size());

		if ((n = Write(p.data(), p.size(), 1)) < 0) {
			throw std::string("write: ") + strerror(errno);
		} else {
			p.resize(0);
		}
	}
	return p;
}



