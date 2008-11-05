/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.

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

*/

/*
Contributors:
Raffi Sevlian, raffisev@gmail.com
*/


#ifndef CM_MESSAGE_H
#define CM_MESSAGE_H


namespace SMS {

/**
	CP-Messages 
*/

class CPMessage : public GSM::L3Message 
{
	
	public:
	CommSig mPayload;  ///< RPDU
		
	/** Message type defined in GSM 04.11 8.1.3 Table 8.1 */
	enum MessageType {
		Data=0x01,
		Ack=0x04,
		Error=0x10
	};


	/* Header information ref. Figure 8. GSM 04.11 */

	unsigned mTIFlag;	///< 0 for originating side, see GSM 04.07 11.2.3.1.3
	unsigned mTIValue; 	///< short transaction ID, GSM 04.07 11.2.3.1.3

	CPMessage( unsigned wTIFlag, unsigned wTIValue )
		:L3Message(),  mTIFlag(wTIFlag), mTIValue(wTIValue)
	{}

	CPMessage( const CommSig wPayload ):
		mPayload(wPayload)
	{ }
	
	
	virtual ~CPMessage(){}
	
	void write( GSM::L3Frame& dest, size_t start=0 )const; 
	GSM::L3PD PD() const { return GSM::L3SMSPD; }

	unsigned TIValue() const { return mTIValue; }
	void TIValue(unsigned wTIValue){ mTIValue=wTIValue; }
	
	unsigned TIFlag() const { return mTIFlag; }
	void TIFlag( unsigned wTIFlag) { mTIFlag=wTIFlag; }
	
	const CommSig payload() const { return mPayload; }
	void text(std::ostream&) const;

};


std::ostream& operator<<(std::ostream& os, CPMessage::MessageType MTI);
CPMessage * parseSMS( const GSM::L3Frame& frame );
CPMessage * CPFactory( CPMessage::MessageType MTI );

class CPAck : public CPMessage 
{	
	public:
	CPAck( unsigned wTIFlag=0, unsigned wTIValue=7)
		:CPMessage(wTIFlag, wTIValue)
 	{ }

	~CPAck(){}
	int MTI() const { return Ack; }
	size_t length() const { return 1; }

	void parseBody( const GSM::L3Frame& dest, size_t &rp ){};
	void writeBody( GSM::L3Frame& dest, size_t &wp ) const{};
	//void text(std::ostream&) const;
};


class CPError : public CPMessage 
{ 
	// default 0x6F Protocol error unspecified
	// ref Table 8.2 GSM 04.11 
	unsigned mCause; 

	public:
	CPError( unsigned wCause=111, unsigned wTIFlag=0, unsigned wTIValue=7)
		:CPMessage(wTIFlag, wTIValue)
 	{ }

	~CPError(){}
	int MTI() const { return Error; }
	size_t length() const { return 1; }
	void writeBody( GSM::L3Frame& dest, size_t &wp ) const;	
};


class CPData : public CPMessage
{
	public:	
	CPData( unsigned wTIFlag=0, unsigned wTIValue=7)
		:CPMessage(wTIFlag, wTIValue)
 	{ }

	CPData( const CommSig wPayload )
		:CPMessage(wPayload)
	{ }

	~CPData(){}
	int MTI() const { return Data; }
	size_t length() const { 
		COUT("CPData::length = "<<mPayload.size()/8)
		return mPayload.size()/8 +1; 
	}
	void parseBody( const GSM::L3Frame& dest, size_t &rp );
	void writeBody( GSM::L3Frame& dest, size_t &wp ) const;
	void text(std::ostream&) const;
};


}; // namespace SMS {
#endif
