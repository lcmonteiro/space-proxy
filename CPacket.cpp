/*
 * File:   CPacket.cpp
 * Author: root
 *
 * Created on December 17, 2014, 9:53 AM
 */
#include <netinet/ip.h>
#include <netinet/ip6.h>

#include <iostream>

#include "CPacket.h"


typedef struct iphdr IP4_HDR, *pIP4_HDR;

typedef struct ip6_hdr IP6_HDR, *pIP6_HDR;

/**
 * 
 */
#define CHECKSUM_CARRY(x) (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))

static uint32_t checksum_math(uint8_t *buf, uint32_t len);

/**
 * 
 */
CBuffer::iterator CBuffer::set(iterator it, int sz) {
	iterator end = it + sz;
	assign(it, end);
	return end;
}

/**
 * 
 */
CPacket::CPacket() : _port_src(2), _port_dst(2), _type(1), _method(1), buffer_t(0, 2000) {
}

CPacket::~CPacket() {
}

void CPacket::Unserialize() {
	uint8_t prolocol = 0;

#ifdef __INFO_PACKET__
	buffer_t::iterator ref = buffer_t::begin() + 4;
#else
	buffer_t::iterator ref = buffer_t::begin();
#endif
	/**
	 * UnSerialize IP
	 */
	switch (((pIP4_HDR) ref.base())->version) {
		case 6:
		{
			ref += 6;
			_type.assign(ref, ref + 1);
			ref += 2;
			_addr_src.assign(ref, ref + 16);
			ref += 16;
			_addr_dst.assign(ref, ref + 16);
			ref += 16;
			break;
		}
		case 4:
		{
			ref += 9;
			_type.assign(ref, ref + 1);
			ref += 3;
			_addr_src.assign(ref, ref + 4);
			ref += 4;
			_addr_dst.assign(ref, ref + 4);
			ref += 4;

			ref += (((pIP4_HDR) buffer_t::data())->ihl - 4) >> 2;
			break;
		}
		default:
		{
			return;
		}
	}
	/**
	 * protocol
	 */
	uint16_t len = 0;

	switch (_type.Integer()) {
			/**
			 * UDP
			 */
		case 0x11:
		{
			_port_src.assign(ref, ref + 2);
			ref += 2;
			_port_dst.assign(ref, ref + 2);
			ref += 2;
			std::copy(ref, ref + 2, (uint8_t*) & len);
			ref += 4;
			_data.assign(ref, ref + ntohs(len) - 8);
			break;
		}
		default:
		{
			return;
		}
	}

}

void CPacket::Serialize(unsigned char version) {

	buffer_t::resize(0);

	/**
	 * Serialize IP header
	 */
	switch (version) {
		case 6:
		{
#ifdef __INFO_PACKET__
			insert(end(), 2, 0x00);
			insert(end(), 1, 0x86);
			insert(end(), 1, 0xdd);
#endif
			insert(end(), version << 4);
			insert(end(), 5, 0);
			insert(end(), _type.begin(), _type.end());
			insert(end(), 1, 0x40);
			insert(end(), _addr_src.begin(), _addr_src.end());
			insert(end(), _addr_dst.begin(), _addr_dst.end());

			break;
		}
		case 4:
		{
#ifdef __INFO_PACKET__
			insert(end(), 2, 0x00);
			insert(end(), 1, 0x08);
			insert(end(), 1, 0x00);
#endif
			insert(end(), 1, 0x45);
			insert(end(), 5, 0);
			insert(end(), 1, 0x40);
			insert(end(), 1, 0);
			insert(end(), 1, 0x40);
			insert(end(), _type.begin(), _type.end());
			insert(end(), 2, 0);
			insert(end(), _addr_src.begin(), _addr_src.end());
			insert(end(), _addr_dst.begin(), _addr_dst.end());
			break;
		}
		default:
		{
			return;
		}
	}
	/**
	 * Serialize Protocol
	 */
	buffer_t len(2);

	switch (_type.Integer()) {
			/**
			 * ICMP
			 */
		case ICMP_T:
		{
			switch (_method.Integer()) {

				default:
				{
					iterator ref = end();

					len.Integer(_data.size() + 6);

					insert(end(), 1, 0x03);
					insert(end(), 1, 0x03);
					insert(end(), 4, 0x00);
					insert(end(), _data.begin(), _data.end());
					/**
					 * checksum
					 */
					uint32_t sum = 0;
					sum += checksum_math(_addr_src.data(), _addr_src.size());
					sum += checksum_math(_addr_dst.data(), _addr_dst.size());
					sum += ntohs(_type.Integer() + len.Integer());
					sum += checksum_math(ref.base(), end().base() - ref.base());
					sum = CHECKSUM_CARRY(sum);

					std::copy((uint8_t*) & sum, ((uint8_t*) & sum) + 2, ref + 2);
				}
			}
			break;
		}
		case ICMPv6_T:
		{
			switch (_method.Integer()) {
				default:
				{
					iterator ref = end();

					len.Integer(_data.size() + 6);

					insert(end(), 1, 0x01);
					insert(end(), 1, 0x04);
					insert(end(), 4, 0x00);
					insert(end(), _data.begin(), _data.end());
					/**
					 * checksum
					 */
					uint32_t sum = 0;
					sum += checksum_math(_addr_src.data(), _addr_src.size());
					sum += checksum_math(_addr_dst.data(), _addr_dst.size());
					sum += ntohs(_type.Integer() + len.Integer());
					sum += checksum_math(ref.base(), end().base() - ref.base());
					sum = CHECKSUM_CARRY(sum);

					std::copy((uint8_t*) & sum, ((uint8_t*) & sum) + 2, ref + 2);
				}
			}
			break;
		}
			/**
			 * UDP
			 */
		case UDP_T:
		{
			iterator ref = end();

			len.Integer(_data.size() + 8);

			insert(end(), _port_src.begin(), _port_src.end());
			insert(end(), _port_dst.begin(), _port_dst.end());
			insert(end(), len.begin(), len.end());
			insert(end(), 2, 0);
			insert(end(), _data.begin(), _data.end());
			/**
			 * checksum
			 */
			uint32_t sum = 0;
			sum += checksum_math(_addr_src.data(), _addr_src.size());
			sum += checksum_math(_addr_dst.data(), _addr_dst.size());
			sum += ntohs(_type.Integer() + len.Integer());
			sum += checksum_math(ref.base(), end().base() - ref.base());
			sum = CHECKSUM_CARRY(sum);

			std::copy((uint8_t*) & sum, ((uint8_t*) & sum) + 2, ref + 6);
			break;
		}
		default:
		{
			return;
		}
	}
	/**
	 * Update IP fields
	 */


	switch (version) {
		case 6:
		{
#ifdef __INFO_PACKET__
			std::copy(len.begin(), len.end(), begin() + 8);
#else
			std::copy(len.begin(), len.end(), begin() + 4);
#endif
			break;
		}
		case 4:
		{
			len.Integer(size());

#ifdef __INFO_PACKET__
			std::copy(len.begin(), len.end(), begin() + 6);
#else
			std::copy(len.begin(), len.end(), begin() + 2);
#endif
			/**
			 * checksum
			 */
			uint32_t sum = 0;
			sum += checksum_math(data(), 20);
			sum = CHECKSUM_CARRY(sum);

			std::copy((uint8_t*) & sum, ((uint8_t*) & sum) + 2, begin() + 10);
			break;
		}
		default:
		{
			return;
		}
	}
}

static uint32_t checksum_math(uint8_t *buf, uint32_t len) {

	uint16_t *data = (uint16_t *) buf;
	uint32_t sum = 0;

	union {
		uint16_t s;
		uint8_t b[2];
	} pad;
	while (len > 1) {
		sum += *data++;
		len -= 2;
	}
	if (len == 1) {
		pad.b[0] = *(uint8_t *) data;
		pad.b[1] = 0;
		sum += pad.s;
	}
	return (sum);
}

