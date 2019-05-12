/* 
 * File:   CTunTap.h
 * Author: root
 *
 * Created on December 11, 2014, 11:08 AM
 */

#ifndef CTUNNELI_H
#define	CTUNNELI_H

#include <string>

#include "CTunnelBase.h"

class CTunnelI : public CTunnelBase {
public:
	CTunnelI(const char* dev, const char* local);

	virtual ~CTunnelI();
	/**
	 * 
	 */
	bool Open();

	bool Close();
	/**
	 */
	virtual CPacket& operator>>(CPacket &p);

	virtual CPacket& operator<<(CPacket &p);

protected:
	std::string _local_addr;

private:
	bool Create();
	
	bool Enable();
};

#endif	/* CTUNTAP_H */

