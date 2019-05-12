/* 
 * File:   CTCPSocket.h
 * Author: root
 *
 * Created on December 17, 2014, 10:24 AM
 */

#ifndef CTUNNELTCP_H
#define	CTUNNELTCP_H

#include "CTunnelBase.h"

class CTunnelTCP : public CTunnelBase {
public:
	CTunnelTCP(const char* dev, int port);

	virtual ~CTunnelTCP();
	/**
	 * 
	 */
	bool Open();

protected:
	/**
	 * 
	 */
	int _port;

private:
	bool Connect();

	bool WaitConnection();
};

#endif	/* CTCPSOCKET_H */

