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


#include "SMSTransfer.h"
#include "SMSMessages.h"

using namespace std;
using namespace GSM;
using namespace SMS;


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



CPMessage * SMS::parseSMS( const GSM::L3Frame& frame )
{
	CPMessage::MessageType MTI = (CPMessage::MessageType)(frame.MTI());	
	CPMessage* retVal;

	try {
		switch(MTI){
			case CPMessage::Data:
				retVal = new CPData(frame); 
				break;	
			case CPMessage::Ack:
				retVal = new CPAck(frame); 
				break;
			case CPMessage::Error:
				retVal = new CPError(frame); 
				break;
			default:
				retVal = NULL;
		}		DCOUT(" parseSMS (CM Layer) "<< *retVal );
	}
	
	catch(const dvectorError &dverr) {
		L3_READ_ERROR;
	}
	return retVal;
}


void CPMessage::text(ostream& os) const 
{
	os <<" CC "<<(CPMessage::MessageType)MTI();
	os <<" TI=("<<mTIFlag<<", "<<mTIValue<<")";
}

/** GSM 04.11  7.2.1 & 8.1.4.1 */
void CPData::writeBody( L3Frame& dest, size_t& wp) const
{ }

