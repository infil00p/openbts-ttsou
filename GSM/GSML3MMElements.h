/**@file
	@brief Elements for Mobility Management messages, GSM 04.08 9.2.
*/

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
David A. Burgess, dburgess@ketsrelsp.com
Raffi Sevlian, raffisev@gmail.com
*/


#ifndef GSML3MMELEMENTS_H
#define GSML3MMELEMENTS_H

#include "GSML3Message.h"

namespace GSM {

/** CM Service Type, GSM 04.08 10.5.3.3 */
class L3CMServiceType : public L3ProtocolElement {

	public:

	enum TypeCode {
		UndefinedType=0,
		MobileOriginatedCall=1,
		EmergencyCall=2,
		ShortMessage=4,
		SupplementaryService=8,
		VoiceCallGroup=9,
		VoiceBroadcast=10,
		LocationService=11,
		MobileTerminatedCall=100			///< non-standard code
	};
		
	private:

	TypeCode mType;

	public:

	L3CMServiceType(TypeCode wType=UndefinedType)
		:L3ProtocolElement(),mType(wType)
	{}

	TypeCode type() const { return mType; }
	
	void parseV( const L3Frame &src, size_t &rp );
	size_t lengthV() const { return 0; }	
	void text(std::ostream&) const;

};


std::ostream& operator<<(std::ostream& os, L3CMServiceType::TypeCode code);


/** RejectCause, GSM 04.08 10.5.3.6 */
class L3RejectCause : public L3ProtocolElement {

private:

	int mRejectCause;

public:
	
	L3RejectCause( const int wRejectCause=0 )
		:L3ProtocolElement(),mRejectCause(wRejectCause)
	{}

	void writeV( L3Frame& dest, size_t &wp ) const;
	size_t lengthV() const { return 1; }	

	void text(std::ostream&) const;
};




/**
	Network Name, GSM 04.08 10.5.3.5a
	This class only supports UCS2.
*/
class L3NetworkName : public L3ProtocolElement {


private:

	static const unsigned maxLen=8;
	char mName[maxLen+1];		///< network name as a C string

public:

	L3NetworkName(const char* wName="")
		:L3ProtocolElement()
	{ strncpy(mName,wName,maxLen); }

	void writeV(L3Frame& dest, size_t &wp) const;
	size_t lengthV() const { return 1+strlen(mName)*2; }

	void text(std::ostream&) const;
};




}

#endif

// vim: ts=4 sw=4
