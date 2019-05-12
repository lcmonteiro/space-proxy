/* 
 * File:   CTCPSocket.cpp
 * Author: root
 * 
 * Created on December 17, 2014, 10:24 AM
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <cstring>
#include <cstdio>

#include <string>

#include "CTunnelTCP.h"

CTunnelTCP::CTunnelTCP(const char* dev, int port) : _port(port), CTunnelBase(dev) {
}

CTunnelTCP::~CTunnelTCP() {
}

/**
 * 
 */
bool CTunnelTCP::Open() {
	if ((_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		ERROR( << "socket():" << strerror(errno));
		return false;
	}
	return _dev.empty() ? WaitConnection() : Connect();
}

/**
 */

bool CTunnelTCP::Connect() {
	/*
	 *  Client, try to connect to server 
	 */
	struct sockaddr_in remote;
	INFO( << "CLIENT: Try to connect to server " << _dev);
	/*
	 *  assign the destination address 
	 */
	std::memset(&remote, 0, sizeof (remote));
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(_dev.data());
	remote.sin_port = htons(_port);

	/* 
	 * connection request 
	 */
	if (connect(_fd, (struct sockaddr*) &remote, sizeof (remote)) < 0) {
		ERROR( << "connect():" << strerror(errno));
		return false;
	}
	INFO( << "CLIENT: Connected to server " << _dev);
	return true;
}

bool CTunnelTCP::WaitConnection() {
	/* 
	 * Server, wait for connections 
	 */
	struct sockaddr_in local, remote;
	int fd, optval = 1;
	socklen_t remotelen;
	INFO( << "SERVER: Waiting connection on port " << _port);
	/*
	 *  avoid EADDRINUSE error on bind() 
	 */
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof (optval)) < 0) {
		ERROR( << "SetSockopt():" << strerror(errno));
		return false;
	}
	memset(&local, 0, sizeof (local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(0);
	local.sin_port = htons(_port);

	if (bind(_fd, (struct sockaddr*) &local, sizeof (local)) < 0) {
		ERROR( << "bind():" << strerror(errno));
		return false;
	}
	if (listen(_fd, 5) < 0) {
		ERROR( << "listen():" << strerror(errno) );
		return false;
	}
	/* 
	 * wait for connection request 
	 */
	remotelen = sizeof (remote);
	memset(&remote, 0, remotelen);
	if ((fd = accept(_fd, (struct sockaddr*) &remote, &remotelen)) < 0) {
		ERROR( << "accept():" << strerror(errno) );
		return false;
	}
	_dev = inet_ntoa(remote.sin_addr);

	/**
	 * close server
	 */
	close(_fd);
	/**
	 * 
	 */
	_fd = fd;

	INFO( << "SERVER: Client connected from " << _dev );
	return true;
}
