/* 
 * File:   CTunnelBase.h
 * Author: root
 *
 * Created on December 19, 2014, 10:18 AM
 */

#ifndef CTUNNELBASE_H
#define	CTUNNELBASE_H

#include <sys/select.h>

#include <ctime>

#include <fstream>

#include "CPacket.h"

//#define __DEBUG__
/**
 * 
 */
#ifdef __DEBUG__
#define DEBUG(x) CTunnelBase::Debug()<<"[ "<<CTunnelBase::DateTime()<<" ] [ DEBUG ] [ "x<<" ]"<<std::endl
#else
#define DEBUG(x)
#endif

#define INFO(x) CTunnelBase::Info()<<"\033[32m"<<"[ "<<CTunnelBase::DateTime()<<" ] [ INFO  ] [ "x<<" ]"<<"\033[0m"<<std::endl

#define ERROR(x) std::cerr<<"\033[31m"<<"[ "<<CTunnelBase::DateTime()<<" ] [ ERROR ] [ "x<<" ]"<<"\033[0m"<<std::endl

#define WARNING(x) std::cerr<<"\033[33m"<<"[ "<<CTunnelBase::DateTime()<<" ] [WARNING] [ "x<<" ]"<<"\033[0m"<<std::endl

#define TRACE_TX(x) CTunnelBase::Trace()<<"\033[34m"<<"[ "<<CTunnelBase::DateTime()<<" ] [  TX   ] [ "x<<" ]"<<"\033[0m"<<std::endl

#define TRACE_RX(x) CTunnelBase::Trace()<<"\033[31m"<<"[ "<<CTunnelBase::DateTime()<<" ] [  RX   ] [ "x<<" ]"<<"\033[0m"<<std::endl

/**
 * 
 */

using namespace std;

class CTunnelBase {
public:
	CTunnelBase(const char* dev);

	virtual ~CTunnelBase();

	/**
	 * 
	 */
	virtual bool Open();

	virtual bool Close();

	/**
	 */
	virtual CPacket& operator>>(CPacket &p);

	virtual CPacket& operator<<(CPacket &p);
	/**
	 * 
	 */
	int Read(unsigned char* buf, int size, int timeout_sec, int timeout_ms = 0);

	int Write(unsigned char* buf, int size, int timeout_sec, int timeout_ms = 0);

	/**
	 * 
	 */
	inline bool EventRX() {
		return Event(_fd);
	}
	/**
	 * 
	 */
	static bool Wait(int timeout, CTunnelBase* pTunn, ...);
	/**
	 * 
	 */
	static void Process();

	/**
	 * 
	 */
	static const string DateTime();

	/**
	 * 
	 */
	static inline ostream& Info() {
		return _info ? cout : _null;
	}
	
	static inline ostream& Debug() {
		return _debug ? cout : _null;
	}

	static inline ostream& Trace() {
		return _trace ? cout : _null;
	}
protected:
	/**
	 * device name
	 */
	string _dev;
	/**
	 * 
	 */
	int _fd;
	/**
	 * 
	 */
	static fd_set _rd_set;
	/**
	 * 
	 */
	static bool _info;
	
	static bool _debug;

	static bool _trace;

	static ofstream _null;
private:

	static inline bool Event(int fd) {
		return (fd >= 0) && FD_ISSET(fd, &_rd_set);
	}
};

#endif	/* CTUNNELBASE_H */

