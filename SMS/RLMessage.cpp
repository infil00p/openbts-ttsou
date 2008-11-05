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



#include "Common.h"
#include "GSML3Message.h"
#include "GSMCommon.h"
#include "GSML3CCElements.h"

#include "SMSTransfer.h"
#include "RLMessage.h"

using namespace std;
using namespace GSM;
using namespace SMS;

#define DEBUG 1


/**
	RP-Messages
*/


ostream& SMS::operator<<(ostream& os, RPMessage::MessageType val)
{
	switch(val) {
		case RPMessage::Data:
			os<<"RP-Data"; break;
		case RPMessage::Ack:
			os<<"RP-Ack"; break;
		case RPMessage::Error:
			os<<"RP-Error"; break;
		case RPMessage::SMMA:
			os<<"RP-SMMA"; break;
		default :
			os<<hex<<"0x"<<(int)val<<dec; break;
	}

	return os;
}

size_t RPMessage::bitsNeeded() const 
{
	DCOUT(" msg_len="<<length()<<" payload="<<mPayload.size())
	size_t retVal = 8 + (length()*8);
	retVal += mPayload.size();
	DCOUT(" bitsNeeded = "<<retVal)
	return retVal;
}

void RPMessage::parse( const RLFrame& frame ){
	size_t rp=8;
	parseBody(frame, rp);
}


void RPData::parseBody( const RLFrame& frame, size_t &rp) 
{
	mMessageReference = frame.readField(rp, 8);
	mOriginatorAddress = frame.readField(rp, 8);
	mDestinationAddress.parseLV(frame, rp);	
	mPayload = frame.segmentAlias(rp);
}
void RPMessage::write( RLFrame& frame ) const {
	size_t wp=5;
	frame.writeField(wp, MTI(), 3); 
	writeBody(frame, wp);
}

void RPData::writeBody( RLFrame& frame, size_t &wp ) const 
{
	frame.writeField(wp, mMessageReference, 8);
	frame.writeField(wp, mOriginatorAddress, 8);

	L3Frame& tmp = (L3Frame&)frame;
	mDestinationAddress.writeLV(tmp, wp);
	mPayload.copyToSegment(frame, wp);		
}


void RPAck::writeBody( RLFrame& frame, size_t &wp ) const 
{
	mPayload.copyToSegment(frame, wp);		
}




RPMessage * SMS::parseRP( const RLFrame& frame )
{
	RPMessage::MessageType MTI = (RPMessage::MessageType)(frame.MTI());	
	RPMessage* retVal;

	DCOUT(" parseRP MTI="<<MTI)
	try {
		switch(MTI){
			case RPMessage::Data:
				retVal = new RPData(); 
				retVal->parse(frame);
				break;	
			//case RPMessage::Ack:
			//	retVal = new CPAck(); 
			//	retVal->parse(frame);
			//	break;
			//case RPMessage::Error:
			//	retVal = new CPError(); 
			//	retVal->parse(frame);
			//	break;
			default:
				return NULL;
		}
		
	}
	
	catch(const dvectorError &dverr) {
		L3_READ_ERROR;
	}
	return retVal;
}



