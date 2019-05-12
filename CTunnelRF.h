/* 
 * File:   CSlipRF.h
 * Author: root
 *
 * Created on December 17, 2014, 10:15 AM
 */

#ifndef CTUNNELRF_H
#define	CTUNNELRF_H

#include <set>

#include "CTunnelSP.h"

/**
 * 
 */
#define DATA_PORT	3001

#define DNS_PORT	53

#define ADD_PORT	47000

#define RM_PORT		47001


/**
 */
typedef std::set<buffer_t> check_t;

class CTunnelRF : public CTunnelSP {

	typedef enum {
		PHY_FRAME_END = 0xC0,
		PHY_FRAME_ESCAPE = 0xDB,
		PHY_TRANSPOSED_FRAME_END = 0xDC,
		PHY_TRANSPOSED_FRAME_ESCAPE = 0xDD
	} PHY_t;

	typedef enum {
		DATA_REQUEST = 1,
		DATA_INDICATION = 2,
		DATA_ERROR = 3,
		REGISTRATION_INDICATION = 4,
		REGISTRATION_RESPONSE = 5,
		DEREGISTRATION_INDICATION = 6,
		REGISTRATION_TABLE_REQUEST = 7,
		REGISTRATION_TABLE_CONFIRM = 8
	} COMMAND_t;

	typedef enum {
		STATE_YES = 0x00,
		STATE_NO = 0x7E,
		STATE_INDETERMINATE = 0x01
	} STATUS_t;

public:
	CTunnelRF(const char* dev, const char* local, const char* list, int speed);

	virtual ~CTunnelRF();
	/**
	 * 
	 */
	bool Open();

	bool Close();

	/**
	 */
	CPacket& operator<<(CPacket &p);

	CPacket& operator>>(CPacket &p);
protected:
	/**
	 * configure
	 */
	std::string _file_name;
	/**
	 * local ipv6 address
	 */
	buffer_t _local_addr;
	/**
	 * 
	 */
	check_t _white_list;

	check_t _join_list;

	check_t _leave_list;

	check_t _reg_list;
	/**
	 * 
	 */

private:
	/**
	 * configuration
	 */
	void UpdateWhiteList();

	/**
	 * process 
	 */
	buffer_t Read();

	void ProcessMsgRx(CPacket &p, buffer_t data);

	buffer_t ProcessMsgTx(CPacket &p);

	void Write(buffer_t data);

	/**
	 * Join and Leave
	 */
	bool Join(buffer_t ip);

	bool Leave(buffer_t ip);

	void ProcessJoin(CPacket &p);

	void ProcessLeave(CPacket &p);
};

#endif	/* CSLIPRF_H */

