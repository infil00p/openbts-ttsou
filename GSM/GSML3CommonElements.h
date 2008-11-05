/**@file
	@brief Common elements for L3 messages, GSM 04.08 10.5.1.
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



#ifndef GSMCOMMONELEMENTS_H
#define GSMCOMMONELEMENTS_H

#include "GSML3Message.h"


namespace GSM {



/** Cell Identity, GSM 04.08 10.5.1.1 */
class L3CellIdentity : public L3ProtocolElement {

	private:

	unsigned mID;

	public:

	L3CellIdentity(unsigned wID=0)
		:mID(wID)
	{ }

	size_t lengthV() const { return 2; }

	void writeV(L3Frame& dest, size_t &wp) const;

	void text(std::ostream&) const;
};





/** LAI, GSM 04.08 10.5.1.3 */
class L3LocationAreaIdentity : public L3ProtocolElement {

	private:

	unsigned mMCC[3];		///< mobile country code digits
	unsigned mMNC[3];		///< mobile network code digits
	unsigned mLAC;			///< location area code


	public:

	L3LocationAreaIdentity():L3ProtocolElement() {}

	/**
		Initialize the LAI with real values.
		@param wMCC MCC as a string (3 digits).
		@param wMNC MNC as a string (2 or 3 digits).
		@param wLAC LAC as a number.
	*/
	L3LocationAreaIdentity(const char*wMCC, const char* wMNC, unsigned wLAC);

	size_t lengthV() const { return 5; }

	void parseV(const L3Frame& source, size_t &rp);
	void writeV(L3Frame& dest, size_t &wp) const;

	void text(std::ostream&) const;
};






/** Mobile Identity, GSM 04.08, 10.5.1.4 */
class L3MobileIdentity : public L3ProtocolElement {

	private:

	
	MobileIDType mType;					///< IMSI, TMSI, or IMEI?
	char mDigits[16];					///< GSM 03.03 2.2 limits the IMSI or IMEI to 15 digits.
	uint32_t mTMSI;						///< GSM 03.03 2.4 specifies the TMSI as 32 bits

	public:

	/** Empty ID */
	L3MobileIdentity()
		:L3ProtocolElement(),
		mType(NoIDType)
	{ mDigits[0]='\0'; } 

	/** TMSI initializer. */
	L3MobileIdentity(unsigned int wTMSI)
		:L3ProtocolElement(),
		mType(TMSIType), mTMSI(wTMSI)
	{ mDigits[0]='\0'; } 

	/** IMSI initializer. */
	L3MobileIdentity(const char* wDigits)
		:L3ProtocolElement(),
		mType(IMSIType)
	{ assert(strlen(wDigits)<=15); strcpy(mDigits,wDigits); }

	/**@name Accessors. */
	//@{
	MobileIDType type() const { return mType; }
	const char* digits() const { assert(mType!=TMSIType); return mDigits; }
	unsigned int TMSI() const { assert(mType==TMSIType); return mTMSI; }
	//@}

	/** Comparison. */
	bool operator==(const L3MobileIdentity&) const;

	size_t lengthV() const;

	void writeV(L3Frame& dest, size_t &wp) const;
	void parseV( const L3Frame& src, size_t &rp, size_t expectedLength );

	void text(std::ostream&) const;
};



} // GSM

#endif

// vim: ts=4 sw=4
