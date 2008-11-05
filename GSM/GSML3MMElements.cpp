/**@file
	@brief Elements for Mobility Management messages, GSM 04.08 9.2.
*/
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
David A. Burgess, dburgess@ketsrelsp.com
Raffi Sevlian, raffisev@gmail.com
*/



#include "GSML3MMElements.h"

using namespace std;
using namespace GSM;


void L3CMServiceType::parseV(const L3Frame& src, size_t &rp)
{
	mType = (TypeCode)src.readField(rp,4);
}


ostream& GSM::operator<<(ostream& os, L3CMServiceType::TypeCode code)
{
	switch (code) {
		case L3CMServiceType::MobileOriginatedCall: os << "MOC"; break;
		case L3CMServiceType::EmergencyCall: os << "Emergency"; break;
		case L3CMServiceType::ShortMessage: os << "SMS"; break;
		case L3CMServiceType::SupplementaryService: os << "SS"; break;
		case L3CMServiceType::VoiceCallGroup: os << "VGCS"; break;
		case L3CMServiceType::VoiceBroadcast: os << "VBS"; break;
		case L3CMServiceType::LocationService: os << "LCS"; break;
		case L3CMServiceType::MobileTerminatedCall: os << "MOT"; break;
		default: os << "?" << (int)code << "?";
	}
	return os;
}

void L3CMServiceType::text(ostream& os) const
{
	os << mType;
}

void L3RejectCause::writeV( L3Frame& dest, size_t &wp ) const
{
	dest.writeField(wp, mRejectCause, 8);
}


void L3RejectCause::text(ostream& os) const
{	
	os <<"0x"<< hex << mRejectCause << dec;	
}





void L3NetworkName::writeV(L3Frame& dest, size_t &wp) const
{
	unsigned sz = strlen(mName);
	// header byte
	// UCS2, no trailing spare bits
	dest.writeField(wp,0x90,8);
	// the characters
	for (unsigned i=0; i<sz; i++) {
		dest.writeField(wp,mName[i],16);
	}
/*
	// FIXME -- 7-bit would be more compact
	// and supported by more handsets
	// GSM 7-bit -- broken
	dest.writeField(wp,0x10,8);
	for (int i=0; i<sz; i++) {
		dest.writeField(wp,encodeGSMChar(mName[i]),7);
	}
*/
}


void L3NetworkName::text(std::ostream& os) const
{
	os << mName;
}

