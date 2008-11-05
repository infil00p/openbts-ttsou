/*
* Copyright 2008 Free Software Foundation, Inc.
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


#ifndef RL_MESSAGE_H
#define RL_MESSAGE_H


namespace SMS {


/**
	RP-Message
*/

class RPMessage 
{
	public:
	CommSig mPayload;  ///< TPDU
	unsigned mMessageReference;

	// ref. Table 8.3 GSM 04.11
	enum MessageType {
		Data=0x0,
		Ack=0x2,
		Error=0x4,
		SMMA=0x6
	};
	RPMessage(){}
	RPMessage( const CommSig& wPayload )
		:mPayload(wPayload)
	{ }

	

	virtual ~RPMessage(){}
	virtual int MTI() const=0;

	virtual size_t length() const=0;
	size_t bitsNeeded() const ;//{ return (8 + (length()+mPayload.size())*8 ); }

	void parse( const RLFrame& frame );
	virtual void parseBody( const RLFrame& frame, size_t &rp) {abort();} 

	void write( RLFrame& frame ) const;
	virtual void writeBody( RLFrame& frame, size_t &rp) const {abort();} 
	const CommSig& payload(){ return mPayload; }


};

std::ostream& operator<<(std::ostream& os, RPMessage::MessageType val);



class RPData : public RPMessage 
{
	public:
	unsigned mOriginatorAddress;
	GSM::L3CalledPartyBCDNumber mDestinationAddress;		
	
	int MTI() const { return Data; }
	RPData(): RPMessage() {}

	void parseBody( const RLFrame& frame, size_t &rp); 		
	void writeBody( RLFrame & frame, size_t &wp )const;
	size_t length() const { return mDestinationAddress.lengthV(); }
};


class RPAck : public RPMessage 
{
	public:
	
	int MTI() const { return Ack; }
	RPAck():RPMessage(){}

	RPAck( const CommSig& frame )
		:RPMessage(frame)
	{ }

	void writeBody( RLFrame & frame, size_t &wp )const;
	size_t length() const { return 0; }
};




RPMessage * parseRP( const RLFrame& frame );




}; // namespace SMS {
#endif
