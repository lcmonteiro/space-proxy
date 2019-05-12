/* 
 * File:   CTunnelSP.h
 * Author: root
 *
 * Created on December 19, 2014, 12:07 PM
 */

#ifndef CTUNNELSP_H
#define	CTUNNELSP_H

#include "CTunnelBase.h"

class CTunnelSP : public CTunnelBase {
public:
	CTunnelSP(const char* dev, int speed);

	virtual ~CTunnelSP();
	/**
	 * 
	 */
	bool Open();
	
private:
	/**
	 * 
	 */
	int _speed;

};

#endif	/* CTUNNELSP_H */

