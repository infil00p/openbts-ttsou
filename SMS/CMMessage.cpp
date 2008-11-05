/*
* Copyright 2008 Free Software Foundation, Inc.
*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*/



#include "GSML3Message.h"
#include "SMSTransfer.h"
#include "CMMessage.h"

using namespace std;
using namespace GSM;
using namespace SMS;

#define DEBUG 1

ostream& SMS::operator<<(ostream& os, CPMessage::MessageType val)
{
	switch(val) {
		case CPMessage::Data:
			os<<"CP-Data"; break;
		case CPMessage::Ack:
			os<<"CP-Ack"; break;
		case CPMessage::Error:
			os<<"CP-Error"; break;
		default :
			os<<hex<<"0x"<<(int)val<<dec; break;
	}

	return os;
}


CPMessage * SMS::CPFactory(CPMessage::MessageType val)
{
	switch(val) {
		case CPMessage::Data: return new CPData();
		case CPMessage::Ack: return new CPAck();
		case CPMessage::Error: return new CPError();
		default: {
			CERR("Warning -- no support for mti="<<val);
			return NULL;
		}
	}	
}



CPMessage * SMS::parseSMS( const GSM::L3Frame& frame )
{
	CPMessage::MessageType MTI = (CPMessage::MessageType)(frame.MTI());	
	DCOUT(" parseSMS MTI="<<MTI)
	
	CPMessage * retVal = CPFactory(MTI);
	if( retVal==NULL ) return NULL;

	try {
		retVal->TIFlag(frame.TIFlag());
		retVal->TIValue(frame.TIValue());
		retVal->parse(frame);
		return retVal;

	}
	
	catch(const dvectorError &dverr) {
		L3_READ_ERROR;
	}

}


void CPMessage::text(ostream& os) const 
{
	os <<" CM "<<(CPMessage::MessageType)MTI();
	os <<" TI=("<<mTIFlag<<", "<<mTIValue<<")";
}



void CPMessage::write(L3Frame& dest, size_t start) const
{
	if (dest.size() != (8*length()+ 16) ) dest.resize( 8*length() + 16);
	size_t wp = start;
	dest.writeField(wp, mTIFlag, 1);
	dest.writeField(wp, mTIValue, 3);
	dest.writeField(wp, PD(), 4);
	dest.writeField(wp, MTI(), 8);

	writeBody(dest, wp);
	dest.fixHistory();
}

void CPData::parseBody( const L3Frame& src, size_t &rp )
{	
	mPayload = src.segmentAlias(24);
	DCOUT("CPData::parse = "<<mPayload<<" "<<mPayload.size())
}

void CPData::writeBody( L3Frame& dest, size_t &wp ) const
{
	mPayload.copyToSegment(dest, 24);	
} 

void CPData::text(ostream& os) const
{
	CPMessage::text(os);
	os<<"RPDU payload = ( "<<mPayload<<" )";
}

void CPError::writeBody( L3Frame& dest, size_t &wp ) const {
	dest.writeField(wp, mCause, 8);	
}



