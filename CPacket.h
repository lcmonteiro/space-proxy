/* 
 * File:   CPacket.h
 * Author: root
 *
 * Created on December 17, 2014, 9:53 AM
 */

#ifndef CPACKET_H
#define	CPACKET_H

#include <cstdio>

#include <vector>
#include <iomanip>
#include <iostream>
#include <sstream>


#define __INFO_PACKET__

typedef class CBuffer : public std::vector<unsigned char> {
public:

	/**
	 * 
	 */
	CBuffer(unsigned char* buf, int size) : std::vector<unsigned char>() {
		assign(buf, buf + size);
	}

	CBuffer(int size, int max_size) : std::vector<unsigned char>(size) {
		if (max_size)reserve(max_size);
	}

	CBuffer(int size) : std::vector<unsigned char>(size) {
	}

	CBuffer() : std::vector<unsigned char>() {
	}

	virtual ~CBuffer() {
	}

	/**
	 * 
	 * 
	 */
	iterator set(iterator it, int sz);

	/**
	 * 
	 */
#ifdef __TS7370__

	inline unsigned char* data() {
		return &(*this)[0];
	}
#endif

	/**
	 * 
	 */
	std::string String(int n = 1, char spacer = ' ') const {
		std::ostringstream result;

		result.width(2);

		int i = n;
		for (const_iterator it = begin(); it != end(); ++it, --i) {
			if (!i) {
				result << spacer;
				i = n;
			}
			result << std::hex << std::setfill('0') << std::setw(2);
			result << (int) *it;
		}
		return result.str();
	}

	int Integer() const {
		int result = 0;

		for (const_iterator i = begin(); i != end(); ++i) {
			result <<= 8;
			result |= (int) *i;
		}
		return result;
	}

	void Integer(int val) {
		for (iterator i = end(); i != begin(); val >>= 8) {

			*(--i) = (unsigned char) val;
		}
	}

} buffer_t;

class CPacket : public buffer_t {
public:

	typedef enum {
		ICMP_T = 0x01, UDP_T = 0x11, ICMPv6_T = 0x3A
	} TYPE;

	typedef enum {
		NOT_FOUND_M
	} METHOD;
public:
	CPacket();

	virtual ~CPacket();

	/**
	 */
	template <class T> inline CPacket& operator>>(T &obj) {
		return obj << *this;
	}

	template <class T> inline CPacket& operator<<(T &obj) {
		return obj >> *this;
	}
	/**
	 */
	void Serialize(unsigned char version);

	void Unserialize();

	/**
	 * 
	 */
	inline iterator IpData() {
#ifdef __INFO_PACKET__
		return begin() + 2;
#else
		return begin();
#endif
	}

	/**
	 * 
	 */
	inline buffer_t& SrcAddress() {
		return _addr_src;
	}

	inline buffer_t& DstAddress() {
		return _addr_dst;
	}

	inline buffer_t& SrcPort() {
		return _port_src;
	}

	inline buffer_t& DstPort() {
		return _port_dst;
	}

	inline buffer_t& Data() {
		return _data;
	}

	inline buffer_t& Type() {
		return _type;
	}

	inline buffer_t& Method() {
		return _method;
	}

protected:

	buffer_t _addr_src;
	buffer_t _addr_dst;
	buffer_t _port_src;
	buffer_t _port_dst;
	buffer_t _data;
	buffer_t _type;
	buffer_t _method;
};

#endif	/* CPACKET_H */

