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

/*
Contributors:
Raffi Sevlian, raffisev@gmail.com
*/


#include "Common.h"
#include "GSML3Message.h"
#include "GSMCommon.h"
#include "GSML3CCElements.h"


#include "SMSTransfer.h"
#include "TLMessage.h"

using namespace std;
using namespace GSM;
using namespace SMS;

#define DEBUG 1

ostream& SMS::operator<<(ostream& os, const TLMessage& msg)
{
	msg.text(os);
	return os;
}


TLMessage *SMS::parseTL( const TLFrame& frame )
{
	TLMessage::MessageType MTI = (TLMessage::MessageType)(frame.MTI());
	TLMessage * retVal;

	DCOUT(" parseRL MTI= "<<MTI)
	
	try {
		switch(MTI) {
			case TLMessage::SMS_Submit:
				retVal = new Submit();
				retVal->parse(frame);
				break;
			default:
				return NULL;
		}
	}
	catch( const dvectorError &dverr ){ 
		L3_READ_ERROR;
	}
	return retVal;
}


void TLMessage::parse( const TLFrame& frame ) 
{
	size_t rp=10;
	parseBody(frame, rp);	
}

void TLMessage::write( TLFrame& frame ) const
{
	size_t wp=0;
	frame.writeField(wp, MTI(), 8);
	writeBody(frame, wp);	
}

void TLMessage::text( ostream& os ) const { } 


size_t TLMessage::bitsNeeded() const 
{
	
	size_t retVal = 8*length()+16;
	DCOUT(" msg_len="<<length()<<" bits="<< retVal)
	return retVal;
}

void Submit::parseBody( const TLFrame& frame, size_t& rp ) 
{
	mRejectDuplicates = frame.readField(rp, 1);
	mValidityPeriodFormat = frame.readField(rp, 2);
	mReplyPath = frame.readField(rp, 1);
	rp+=2;
	DCOUT("rp ="<<rp)
	mMessageReference = frame.readField(rp, 8);
	mDestinationAddress.parseLV(frame, rp);

	mProtocolIdentifier = frame.readField(rp, 8);
	mDataCodingScheme = frame.readField(rp, 8);
	mValidityPeriod = frame.readField(rp, 8);
	mUserDataLength = frame.readField(rp, 8);

	TLFrame tmp = const_cast<TLFrame&>(frame);		
	tmp.LSB8MSB();

	for ( unsigned k=0; k<mUserDataLength; k++) {	
		TLFrame tmp0 = tmp.segmentAlias(rp, 8);
		tmp0.LSB8MSB();
		size_t t=1;
		txt_msg[k] = tmp0.readField(t, 7);
		rp += 7;
	}
}

void Submit::text( ostream& os ) const
{
	os << " RejectDuplicates =( "<<mRejectDuplicates<<"), ";
	os << " ValidityPeriodFormat = ("<<mValidityPeriodFormat<<"), ";		
	os << " ReplyPath = ("<<mReplyPath<<"), ";
	os << " MessageReference = ("<<mMessageReference<<" )";
	os << " DestinationAddress = ("<<mDestinationAddress<<" )";
	os << " ValidityPeriod = ("<<mValidityPeriod<<" )";
	os << " UserDataLength = ("<<mUserDataLength<<" )";
	os << " UserData = ( ";
	for( unsigned k=0;k<mUserDataLength;k++){
		os<<hex<<txt_msg[k]<<" ";
	}
	os << " ) ";

}

void SubmitReport::writeBody( TLFrame& frame, size_t &wp ) const
{
	// write remainder of octet.
	wp+=6;
	
	// write Parameter indicator= 0
	wp+=8;
	
	// write Timestamp
	mSCTS.write((L3Frame&)frame, wp);
}

void SubmitReport::text( ostream& os ) const 
{
	os<<" Timestamp = ("<<mSCTS<<")";
}

 
