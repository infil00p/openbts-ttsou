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



#ifndef SMS_MESSAGE_H
#define SMS_MESSAGE_H

#include "Common.h"
#include "SMSTransfer.h"


namespace SMS {

/** virtual base class for following CPMessages 
(CP-Data, CP-Ack, CP-Error) */
class CPMessage : public GSM::L3Message 
{
	
	public:
	
	/** Message type defined in GSM 04.11 8.1.3 Table 8.1 */
	enum MessageType {
		Data=0x01,
		Ack=0x04,
		Error=0x10
	};


	/* Header information ref. Figure 8. GSM 04.11 */

	unsigned mTIFlag;	///< 0 for originating side, see GSM 04.07 11.2.3.1.3
	unsigned mTIValue; 	///< short transaction ID, GSM 04.07 11.2.3.1.3

	CPMessage()
	{ }
	
	virtual ~CPMessage(){}
	
	void write( GSM::L3Frame& dest, size_t start=0 )const; 
	GSM::L3PD PD() const { return GSM::L3SMSPD; }

	unsigned TIValue() const { return mTIValue; }
	void TIValue(unsigned wTIValue){ mTIValue=wTIValue; }
	
	unsigned TIFlag() const { return mTIFlag; }
	void TIFlag( unsigned wTIFlag) { mTIFlag=wTIFlag; }
	
	void text(std::ostream&) const;

};


std::ostream& operator<<(std::ostream& os, CPMessage::MessageType MTI);

CPMessage * parseSMS( const GSM::L3Frame& frame );


class CPAck : public CPMessage 
{	
	public:
	CPAck( const GSM::L3Frame& src ): CPMessage()
 	{ }

	~CPAck(){}
	int MTI() const { return Ack; }
	size_t length() const { return 0; }
};


class CPError : public CPMessage 
{
	unsigned mCause;

	public:
	CPError( const GSM::L3Frame& src ): CPMessage()
	{ }

	~CPError(){}
	int MTI() const { return Error; }
	size_t length() const { return 0; }
};


class CPData : public CPMessage
{
	public:	
	RLFrame mPayload;
	CPData( const GSM::L3Frame& src ): CPMessage()
	{ 
		// get RPDU (according to spec it at 32 bits)
		mPayload = src.segmentAlias(4*8);
	}

	CPData( const RLFrame& wPayload ): 
		CPMessage(), mPayload(wPayload)
	{ }
	


	~CPData(){}
	int MTI() const { return Data; }
	size_t length() const { return 0; }
	void parseBody( const GSM::L3Frame& src, size_t &rp );
	void writeBody( GSM::L3Frame& dest, size_t &wp ) const; 
};


}; // namespace SMS {
#endif
