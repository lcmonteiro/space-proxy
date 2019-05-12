/* 
 * File:   CTunnelNull.h
 * Author: Luis Monteiro
 *
 * Created on December 19, 2014, 12:07 PM
 */

#ifndef CTUNNELNULL_H
#define	CTUNNELNULL_H

#include "CTunnelBase.h"

class CTunnelNull : public CTunnelBase {
public:

	CTunnelNull() : CTunnelBase("null") {
	}

	virtual ~CTunnelNull() {
	}
};

#endif	/* CTUNNELNULL_H */

