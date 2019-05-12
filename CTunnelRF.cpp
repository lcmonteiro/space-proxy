/*
 * File:   CSlipRF.cpp
 * Author: root
 *
 * Created on December 17, 2014, 10:16 AM
 */
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <cstring>

#include <iostream>
#include <fstream>
#include <algorithm>

#include "CTunnelRF.h"

//#define __TEST__

CTunnelRF::CTunnelRF(const char* dev, const char* local, const char* list, int speed) : _file_name(list), _local_addr(16), CTunnelSP(dev, speed) {
	std::string address = local;

	std::replace(address.begin(), address.end(), '/', '\0');

	if (inet_pton(AF_INET6, address.c_str(), _local_addr.data()) <= 0) {
		if (inet_pton(AF_INET, address.c_str(), _local_addr.data()) <= 0) {
			_local_addr.clear();
		}
		_local_addr.resize(4);
	}

#ifdef __TEST__
	buffer_t ip(16);
	if (inet_pton(AF_INET6, "2001:1::100", ip.data()) > 0) {
		_join_list.insert(ip);
	}
#endif
}

CTunnelRF::~CTunnelRF() {
}

/**
 *
 */
bool CTunnelRF::Open() {
	/**
	 * set white list
	 */
	UpdateWhiteList();
	/*
	 * open serial port
	 */

	if (!CTunnelSP::Open()) {
		return false;
	}
	/**
	 * Registration Table Request
	 */
	buffer_t request(0, 128);
	request.insert(request.end(), 1, REGISTRATION_TABLE_REQUEST);
	request.insert(request.end(), 2, 0);
	request.insert(request.end(), 1, 0);
	request.insert(request.end(), 1, 0x10);

	Write(request);
	return true;
}

bool CTunnelRF::Close() {
	return CTunnelSP::Close();
}

/**
 *
 */
CPacket& CTunnelRF::operator<<(CPacket &p) {

	if (!p.empty()) {
		Write(ProcessMsgTx(p));
	}
	return p;
}

CPacket& CTunnelRF::operator>>(CPacket &p) {

	ProcessMsgRx(p, Read());

	if (p.empty()) {
		ProcessJoin(p);
	}
	if (p.empty()) {
		ProcessLeave(p);
	}
	return p;
}

void CTunnelRF::UpdateWhiteList() {

	std::ifstream file(_file_name.data());

	INFO( << "file: " << _file_name);

	for (std::string line; getline(file, line);) {
		buffer_t ip(16);
		if (inet_pton(AF_INET6, line.c_str(), ip.data()) > 0) {
			_white_list.insert(ip);
		}
	}
	/**
	 *
	 */
	INFO( << "white list:");
	for (check_t::iterator it = _white_list.begin(); it != _white_list.end(); ++it) {
		INFO( << it->String());
	}
}

buffer_t CTunnelRF::Read() {
	buffer_t data;
	/**
	 * check event
	 */
	if (!EventRX()) {
		return data;
	}
	/**
	 * reserve buffer
	 */
	data.reserve(1500);

	/**
	 *  1st - search for the first frame end
	 */
	unsigned char d = 0;
	int n = 0;

	while ((n = CTunnelBase::Read(&d, sizeof (d), 1)) > 0) {
		if (d == (unsigned char) PHY_FRAME_END) {
			break;
		}
	}
	if (n <= 0) {
		ERROR( << "RF::First flag not found, framing error.");
		data.resize(0);
		return data;
	}
	/**
	 *  2nd - Process the rest of the frame
	 */
	while ((n = CTunnelBase::Read(&d, sizeof (d), 1)) > 0) {

		switch (d) {
			case(PHY_FRAME_END):
			{
				TRACE_RX("c0 " << data.String() << " c0");
				return data;
			}
			case(PHY_FRAME_ESCAPE):
			{
				if ((n = CTunnelBase::Read(&d, sizeof (d), 1)) > 0) {
					switch (d) {
						case(PHY_TRANSPOSED_FRAME_END):
						{
							data.insert(data.end(), PHY_FRAME_END);
							break;
						}
						case(PHY_TRANSPOSED_FRAME_ESCAPE):
						{
							data.insert(data.end(), PHY_FRAME_ESCAPE);
							break;
						}
						default:
						{
							ERROR( << "RF::Invalid escape, framing error.");
							data.resize(0);
							return data;
						}
					}
				}
				break;
			}
			default:
			{
				data.insert(data.end(), d);
			}
		}
	}
	ERROR( << "RF::Last flag not found, framing error.");
	data.resize(0);
	return data;
}

void CTunnelRF::ProcessMsgRx(CPacket &p, buffer_t data) {

	if (data.empty()) {
		return;
	}
	if (data.size() < 7) {
		ERROR( << "RF::invalid primitive frame size:" << data.size());
		return;
	}
	switch (data.front()) {

		case(DATA_REQUEST): // data request
		{
			ERROR( << "RF::UNEXPECTED DATA.request.");
			break;
		}
		case(DATA_INDICATION): // data indication
		{
			if (data.size() < 17) {
				ERROR( << "RF::invalid DATA_INDICATION frame size:" << data.size());
				return;
			}
			p.SrcAddress().assign(data.begin() + 1, data.begin() + 17);

			p.DstAddress() = _local_addr;

			p.SrcPort().Integer(DATA_PORT);

			p.DstPort().Integer(DATA_PORT);

			p.Data().assign(data.begin() + 17, data.end());

			p.Type().Integer(CPacket::UDP_T);

			p.Serialize(6);

			TRACE_RX( << "IP:" << p.SrcAddress().String(2, ':'));
			break;
		}
		case(DATA_ERROR): // data error
		{
			if (data.size() < 17) {
				ERROR( << "RF::INVALID DATA.error function (data size < 17)");
				return;
			}
			/**
			 * return Destination unreachable 
			 */
			p.SrcAddress().assign(data.begin() + 1, data.begin() + 17);

			p.DstAddress() = _local_addr;

			p.Type().Integer(CPacket::ICMPv6_T);

			p.Method().Integer(CPacket::NOT_FOUND_M);

			p.Data().clear();

			p.Serialize(6);

			ERROR( << "RF::INVALID DATA.error function (IP: " << p.SrcAddress().String(2, ':') << ")");
			break;
		}
		case(REGISTRATION_INDICATION): // registration indication
		{
			if (data.size() < 17) {
				ERROR( << "RF::invalid REGISTRATION_INDICATION frame size:" << data.size());
				return;
			}
			buffer_t ip;
			ip.set(data.begin() + 1, 16);
			/**
			 * registration response
			 */
			buffer_t response(0, 128);
			response.insert(response.end(), 1, REGISTRATION_RESPONSE);
			response.insert(response.end(), ip.begin(), ip.end());
			if (Join(ip)) {
				response.insert(response.end(), 1, STATE_YES);
			} else {
				response.insert(response.end(), 1, STATE_NO);
			}
			Write(response);
			break;
		}
		case(REGISTRATION_RESPONSE): // registration response
		{
			ERROR( << "RF::UNEXPECTED REGISTRATION.response.");
			break;
		}
		case(DEREGISTRATION_INDICATION):
		{
			if (data.size() < 17) {
				ERROR( << "RF::invalid DEREGISTRATION_INDICATION frame size:" << data.size());
				return;
			}
			buffer_t ip;
			ip.set(data.begin() + 1, 16);

			if (!Leave(ip)) {
				ERROR( << "RF::DEREGISTRATION_INDICATION (Address :" << ip.String(2, ':') << ") not found on white list) ");
			}
			break;
		}
		case(REGISTRATION_TABLE_REQUEST):
		{
			ERROR( << "RF::UNEXPECTED REGISTRATION_TABLE.request.");
			break;

		}
		case(REGISTRATION_TABLE_CONFIRM):
		{
			buffer_t::iterator ref = data.begin() + 1;
			buffer_t nbentries;
			buffer_t lastinlist;
			buffer_t lastintable;

			ref = nbentries.set(ref, 2);
			ref = lastinlist.set(ref, 2);
			ref = lastintable.set(ref, 2);


			for (int i = 0, n = nbentries.Integer(); i < n; i++) {
				buffer_t ip;
				buffer_t status;

				ref = ip.set(ref, 16);
				ref = status.set(ref, 1);

				buffer_t response(0, 128);
				response.insert(response.end(), 1, REGISTRATION_RESPONSE);
				response.insert(response.end(), ip.begin(), ip.end());

				INFO( << "IP: " << ip.String(2, ':') << " status: " << status.Integer());

				switch (status.Integer()) {
					case STATE_YES:
					{
						if (!Join(ip)) {
							response.insert(response.end(), 1, STATE_NO);
							Write(response);
						}
						break;
					}
					default:
					{
						if (!Join(ip)) {
							response.insert(response.end(), 1, STATE_NO);

						} else {
							response.insert(response.end(), 1, STATE_YES);
						}
						Write(response);
					}
				}
			}

			DEBUG( << "lastinlist = " << lastinlist.Integer() << " lastintable = " << lastintable.Integer());

			if (lastinlist.Integer() < lastintable.Integer()) {
				buffer_t request(0, 128);

				lastinlist.Integer(lastinlist.Integer() + 1);
				/**
				 * Get next block
				 */
				request.insert(request.end(), 1, REGISTRATION_TABLE_REQUEST);
				request.insert(request.end(), lastinlist.begin(), lastinlist.end());
				request.insert(request.end(), 1, 0);
				request.insert(request.end(), 1, 0x10);

				Write(request);
			}
			break;
		}
		default:
		{
			ERROR( << "RF::INVALID function code received.");
			break;
		}
	}
}

buffer_t CTunnelRF::ProcessMsgTx(CPacket &p) {

	buffer_t out(0, 1500);

	/**
	 *
	 */
	p.Unserialize();
	/**
	 *
	 */
	switch (p.Type().Integer()) {
		case 0x11:
		{
			switch (p.SrcPort().Integer()) {
				case(DNS_PORT):
				{
					switch (p.DstPort().Integer()) {
						case ADD_PORT:
						{
							_join_list.erase(p.DstAddress());
							break;
						}
						case RM_PORT:
						{
							_leave_list.erase(p.DstAddress());
							break;
						}
					}
					p.clear();
					break;
				}
				default:
				{
					if (_reg_list.find(p.DstAddress()) != _reg_list.end()) {

						TRACE_TX( << "IP:" << p.DstAddress().String(2, ':'));

						out.insert(out.end(), 1, DATA_REQUEST);
						out.insert(out.end(), p.DstAddress().begin(), p.DstAddress().end());
						out.insert(out.end(), p.Data().begin(), p.Data().end());
						p.clear();

					} else {
						WARNING( << "RF::Address not found (IP: " << p.DstAddress().String(2, ':') << ")");
						/**
						 * return Destination unreachable 
						 */
						swap(p.DstAddress(), p.SrcAddress());
						p.Type().Integer(CPacket::ICMPv6_T);
						p.Method().Integer(CPacket::NOT_FOUND_M);
						p.Data().assign(p.IpData(), p.end());
						p.Serialize(6);
					}
					break;
				}
			}
			break;
		}
		default:
		{
			DEBUG( << "RF::Ignore MSG (type: " << p.Type().Integer() << ")");
		}
	}
	return out;
}

void CTunnelRF::Write(buffer_t data) {
	buffer_t out(0, 1500);

	if (data.empty()) {
		return;
	}

	out.push_back(PHY_FRAME_END);

	for (int i = 0; i < data.size(); i++) {

		unsigned char c = data.at(i);

		switch (c) {
			case(PHY_FRAME_END):
			{
				out.push_back(PHY_FRAME_ESCAPE);
				out.push_back(PHY_TRANSPOSED_FRAME_END);
				break;
			}
			case(PHY_FRAME_ESCAPE):
			{
				out.push_back(PHY_FRAME_ESCAPE);
				out.push_back(PHY_TRANSPOSED_FRAME_ESCAPE);
				break;
			}
			default:
			{
				out.push_back(c);
				break;
			}
		}
	}
	out.push_back(PHY_FRAME_END);

	/**
	 *  write to physical interface
	 */
	TRACE_TX( << out.String());

	int n = 0;

	if ((n = CTunnelBase::Write(out.data(), out.size(), 1)) < 0) {
		throw std::string("write: ") + strerror(errno);
	}
}

bool CTunnelRF::Join(buffer_t ip) {
	if (_white_list.find(ip) != _white_list.end()) {
		_join_list.insert(ip);
		_reg_list.insert(ip);
		_leave_list.erase(ip);
		return true;
	}
	_leave_list.insert(ip);

	return false;
}

bool CTunnelRF::Leave(buffer_t ip) {
	if (_white_list.find(ip) != _white_list.end()) {
		_leave_list.insert(ip);
		_reg_list.erase(ip);
		_join_list.erase(ip);
		return true;
	}
	return false;
}

void CTunnelRF::ProcessJoin(CPacket &p) {

	if (_join_list.empty()) {
		return;
	}
	/**
	 * get first address
	 */
	buffer_t addr = *_join_list.begin();
	/**
	 * 
	 */
	p.DstAddress() = _local_addr;
	p.SrcAddress() = addr;
	p.DstPort().Integer(DNS_PORT);
	p.SrcPort().Integer(ADD_PORT);
	p.Type().Integer(CPacket::UDP_T);
	/**
	 * 
	 * DNS update add (zone= rf.efacec.com)
	 * 
	 */
	static unsigned char dns_update[] = {
		0x96, 0xbe, 0x28, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x72, 0x66, 0x06,
		0x65, 0x66, 0x61, 0x63, 0x65, 0x63, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x06, 0x00, 0x01, 0x27,
		0x32, 0x30, 0x30, 0x31, 0x3a, 0x30, 0x30, 0x30, 0x31, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30,
		0x30, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30, 0x30,
		0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x31, 0x00, 0x00, 0x1c, 0x00, 0x01, 0x00, 0x01, 0x51, 0x80,
		0x00, 0x10, 0x20, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01
	};
	/**
	 * 
	 */
	std::string name = addr.String(2, ':');

	/**
	 * data
	 */
	buffer_t msg(dns_update, sizeof (dns_update));

	std::copy(name.begin(), name.end(), msg.begin() + 32);

	std::copy(addr.begin(), addr.end(), msg.begin() + 82);

	p.Data() = msg;

	p.Serialize(_local_addr.size() == 4 ? 4 : 6);

	INFO( << "join: " << name);
}

void CTunnelRF::ProcessLeave(CPacket &p) {
	if (_leave_list.empty()) {
		return;
	}
	/**
	 * get first address
	 */
	buffer_t addr = *_leave_list.begin();
	/**
	 * 
	 */
	p.DstAddress() = _local_addr;
	p.SrcAddress() = addr;
	p.DstPort().Integer(DNS_PORT);
	p.SrcPort().Integer(RM_PORT);
	p.Type().Integer(CPacket::UDP_T);
	/**
	 * 
	 * DNS update delete (zone= rf.efacec.com)
	 * 
	 */
	static unsigned char dns_update[] = {
		0x15, 0x73, 0x28, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0x72, 0x66, 0x06,
		0x65, 0x66, 0x61, 0x63, 0x65, 0x63, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x06, 0x00, 0x01, 0x27,
		0x32, 0x30, 0x30, 0x31, 0x3a, 0x30, 0x30, 0x30, 0x31, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30,
		0x30, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x3a, 0x30, 0x30,
		0x30, 0x30, 0x3a, 0x30, 0x30, 0x30, 0x31, 0x00, 0x00, 0x1c, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00
	};
	/**
	 * 
	 */
	std::string name = addr.String(2, ':');

	/**
	 * data
	 */
	buffer_t msg(dns_update, sizeof (dns_update));

	std::copy(name.begin(), name.end(), msg.begin() + 32);

	p.Data() = msg;

	p.Serialize(6);

	INFO( << "leave: " << name);
}
