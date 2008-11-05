/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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


#ifndef TL_MESSAGE_H
#define TL_MESSAGE_H


namespace SMS {

/**
	TL-Messages 
*/

class TLMessage 
{
	public:
	enum MessageType {
		SMS_Submit=0x0,
		SMS_SubmitReport=0x0
	};

	TLMessage(){}
	virtual ~TLMessage(){}
	
	MessageType mMTI;
	
	virtual unsigned MTI() const { return mMTI; }
	// return number of octets
	virtual size_t length() const =0;
	virtual void parseBody( const TLFrame& frame, size_t &rp ){};
	void parse( const TLFrame& frame );
	
	virtual void writeBody( TLFrame& frame, size_t &rp ) const{ abort(); }
	void write( TLFrame& frame ) const;

	virtual	void text( std::ostream& os ) const;	
	
	// 8* #octets + 16 bits for 2 headers
	size_t bitsNeeded() const;
};

std::ostream& operator<<(std::ostream& os, const TLMessage& msg);



class Submit : public TLMessage 
{
	public:

	// Octet 1
	unsigned mRejectDuplicates;
	unsigned mValidityPeriodFormat;
	unsigned mReplyPath;	

	// Octet 2
	unsigned mMessageReference;

	// Octet 3-N
	GSM::L3CalledPartyBCDNumber mDestinationAddress;	

	// Octet 4
	unsigned mProtocolIdentifier;	

	// Octet 5
	unsigned mDataCodingScheme;

	// Octet 6
	unsigned mValidityPeriod;

	// Octet 7
	unsigned mUserDataLength;
	// Octet 8+
	unsigned txt_msg[160];
	
	Submit():TLMessage(){}	
	~Submit(){}

	void parseBody( const TLFrame& frame, size_t& rp ); 
	virtual void text( std::ostream& os ) const ;
	size_t length() const { return 8+mDestinationAddress.lengthV(); }
};

class SubmitReport : public TLMessage
{
	public:
	GSM::L3BCDDigits mSCTS;
		
	SubmitReport(){}

	SubmitReport( const char * wDigits )
		:mSCTS(wDigits)
	{ }

	~SubmitReport(){}
	void writeBody( TLFrame& frame, size_t& wp ) const ;
	virtual void text( std::ostream& os ) const ;
	// Timestamp + 2 header octets.
	size_t length() const { return mSCTS.size()+2; }
};


TLMessage * parseTL( const TLFrame& frame );




}; // namespace SMS {
#endif
