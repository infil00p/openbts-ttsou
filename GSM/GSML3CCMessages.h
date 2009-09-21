/**@file Messages for Call Control, GSM 04.08 9.3. */

/*
* Copyright 2008 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

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



#ifndef GSML3CCMESSAGES_H
#define GSML3CCMESSAGES_H

#include "GSMCommon.h"
#include "GSML3Message.h"
#include "GSML3CommonElements.h"
#include "GSML3CCElements.h"


namespace GSM { 



/**
	This a virtual class for L3 messages in the Call Control protocol.
	These messages are defined in GSM 04.08 9.3.

	GSM call control is based on and nearly identical to ISDN call control.
	ISDN call control is defined in ITU-T Q.931.
*/
class L3CCMessage : public L3Message {

	private:

	unsigned mTIValue;		///< short transaction ID, GSM 04.07 11.2.3.1.3.
	unsigned mTIFlag;		///< 0 for originating side, see GSM 04.07 11.2.3.1.3.

	public:

	/** GSM 04.08, Table 10.3, bit 6 is "don't care" */
	enum MessageType {
		/**@name call establishment */
		//@{
		Alerting=0x01,
		CallConfirmed=0x08,
		CallProceeding=0x02,
		Connect=0x07,
		Setup=0x05,
		ConnectAcknowledge=0x0f,
		Progress=0x03,
		//@}
		/**@name call clearing */
		//@{
		Disconnect=0x25,
		Release=0x2d,
		ReleaseComplete=0x2a,
		//@}
		/**@name DTMF */
		//@{
		StartDTMF=0x35,
		StopDTMF=0x31,
		StopDTMFAcknowledge=0x32,
		StartDTMFAcknowledge=0x36,
		StartDTMFReject=0x37,
		//@}
		/**@name error reporting */
		//@{
		CCStatus= 0x3d
		//@}
	};

	L3CCMessage(unsigned wTIFlag, unsigned wTIValue)
		:L3Message(),mTIValue(wTIValue),mTIFlag(wTIFlag)
	{}


	/** Override the write method to include transaction identifiers in header. */
	void write(L3Frame& dest) const;

	L3PD PD() const { return L3CallControlPD; }

	unsigned TIValue() const { return mTIValue; }
	void TIValue(unsigned wTIValue) { mTIValue = wTIValue; }

	unsigned TIFlag() const { return mTIFlag; }
	void TIFlag(unsigned wTIFlag) { mTIFlag = wTIFlag; }

	void text(std::ostream&) const;
};


std::ostream& operator<<(std::ostream& os, L3CCMessage::MessageType MTI);


/**
	Parse a complete L3 call control message into its object type.
	@param source The L3 bits.
	@return A pointer to a new message or NULL on failure.
*/
L3CCMessage* parseL3CC(const L3Frame& source);

/**
	A Factory function to return a L3CCMessage of the specified MTI.
	Returns NULL if the MTI is not supported.
*/
L3CCMessage* L3CCFactory(L3CCMessage::MessageType MTI);


/** GSM 04.08 9.3.19 */
class L3Release : public L3CCMessage {

	private:

	// We're ignoring "facility" and "user-user" for now.
	bool mHaveCause;
	L3Cause mCause;

	public:

	L3Release(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveCause(false)
	{}

	L3Release(unsigned wTIFlag, unsigned wTIValue, const L3Cause& wCause)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveCause(true),mCause(wCause)
	{}

	int MTI() const { return Release; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const;

	void text(std::ostream&) const;

};


class L3CCStatus : public L3CCMessage 
{
private:
	L3Cause mCause;
	L3CallState mCallState;
public:

	L3CCStatus(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	L3CCStatus(unsigned wTIFlag, unsigned wTIValue, 
		const L3Cause &wCause, const L3CallState &wCallState)
		:L3CCMessage(wTIFlag,wTIValue), 
		mCause(wCause), 
		mCallState(wCallState)
	{}
	
	int MTI() const { return CCStatus; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const { return 4; }

	void text(std::ostream&) const;
};



/** GSM 04.08 9.3.19 */
class L3ReleaseComplete : public L3CCMessage {

	private:

	// We're ignoring "facility" and "user-user" for now.
	bool mHaveCause;
	L3Cause mCause;

	public:

	L3ReleaseComplete(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveCause(false)
	{}

	L3ReleaseComplete(unsigned wTIFlag, unsigned wTIValue, const L3Cause& wCause)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveCause(true),mCause(wCause)
	{}

	int MTI() const { return ReleaseComplete; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const;

	void text(std::ostream&) const;

};





/**
	GSM 04.08 9.3.23
	This message can have different forms for uplink and downlink
	but the TLV format is flexiable enough to allow us to use one class for both.
*/
class L3Setup : public L3CCMessage
{

	// We fill in IEs one at a time as we need them.

	/// Bearer Capability IE
	bool mHaveBearerCapability;
	L3BearerCapability mBearerCapability;

	/// Calling Party BCD Number (0x5C O TLV 3-19 ).
	bool mHaveCallingPartyBCDNumber;
	L3CallingPartyBCDNumber mCallingPartyBCDNumber;

	/// Called Party BCD Number (0x5E O TLV 3-19).
	bool mHaveCalledPartyBCDNumber;
	L3CalledPartyBCDNumber mCalledPartyBCDNumber;



public:

	L3Setup(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveBearerCapability(false),
		mHaveCallingPartyBCDNumber(false),
		mHaveCalledPartyBCDNumber(false)
	{ }

	
	L3Setup(unsigned wTIFlag, unsigned wTIValue, const L3CalledPartyBCDNumber& wCalledPartyBCDNumber)
		:L3CCMessage(wTIFlag,wTIValue), 
		mHaveBearerCapability(false),
		mHaveCallingPartyBCDNumber(false),
		mHaveCalledPartyBCDNumber(true),mCalledPartyBCDNumber(wCalledPartyBCDNumber)
	{ }
	
	L3Setup(unsigned wTIFlag, unsigned wTIValue, const L3CallingPartyBCDNumber& wCallingPartyBCDNumber)
		:L3CCMessage(wTIFlag,wTIValue), 
		mHaveBearerCapability(false),
		mHaveCallingPartyBCDNumber(true),mCallingPartyBCDNumber(wCallingPartyBCDNumber),
		mHaveCalledPartyBCDNumber(false)
	{ }




	/** Accessors */
	//@{
	bool haveCalledPartyBCDNumber() const { return mHaveCalledPartyBCDNumber; }

	const L3CalledPartyBCDNumber& calledPartyBCDNumber() const
		{ assert(mHaveCalledPartyBCDNumber); return mCalledPartyBCDNumber; }
	//@}

	int MTI() const { return Setup; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );

	size_t bodyLength() const;

	void text(std::ostream&) const;
};



/** GSM 04.08 9.3.3 */
class L3CallProceeding : public L3CCMessage {

	private:

	// We'fill in IEs one at a time as we need them.

	bool mHaveBearerCapability;
	L3BearerCapability mBearerCapability;

	bool mHaveProgress;
	L3ProgressIndicator mProgress;

public:

	L3CallProceeding(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveBearerCapability(false),
		mHaveProgress(false)
	{}
	
	int MTI() const { return CallProceeding; }
	void writeBody( L3Frame & dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &wp );
	size_t bodyLength() const;

	void text(std::ostream&) const;
};




/**
	GSM 04.08 9.3.1.
	Even though uplink and downlink forms have different optional fields,
	we can use a single message for both sides.
*/
class L3Alerting : public L3CCMessage 
{
	private:

	bool mHaveProgress;
	L3ProgressIndicator mProgress; 		///< Progress appears in uplink only.

	public:

	L3Alerting(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveProgress(false)
	{}

	L3Alerting(unsigned wTIFlag, unsigned wTIValue,const L3ProgressIndicator& wProgress)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveProgress(true),mProgress(wProgress)
	{}

	int MTI() const { return Alerting; }
	void writeBody(L3Frame &dest, size_t &wp) const;
	void parseBody(const L3Frame& src, size_t &rp);
	size_t bodyLength() const;

	void text(std::ostream&) const;
};




/** GSM 04.08 9.3.5 */
class L3Connect : public L3CCMessage 
{
	private:

	bool mHaveProgress;
	L3ProgressIndicator mProgress; 		///< Progress appears in uplink only.

	public:

	L3Connect()
		:L3CCMessage(0,7),
		mHaveProgress(false)
	{}

	L3Connect(unsigned wTIFlag, unsigned wTIValue)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveProgress(false)
	{}

	L3Connect(unsigned wTIFlag, unsigned wTIValue, const L3ProgressIndicator& wProgress)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveProgress(true),mProgress(wProgress)
	{}

	int MTI() const { return Connect; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody(const L3Frame &src, size_t &wp);
	size_t bodyLength() const;
	void text(std::ostream&) const;
};




/** GSM 04.08 9.3.6 */
class L3ConnectAcknowledge : public L3CCMessage 
{
public:

	L3ConnectAcknowledge(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	int MTI() const { return ConnectAcknowledge; }
	void writeBody( L3Frame &dest, size_t &wp ) const {}
	void parseBody( const L3Frame &src, size_t &rp ) {}
	size_t bodyLength() const { return 0; }

};


/** GSM 04.08 9.3.7 */
class L3Disconnect : public L3CCMessage {

private:

	L3Cause mCause;

public:

	/** Initialize with default cause of 0x10 "normal call clearing". */
	L3Disconnect(unsigned wTIFlag=0, unsigned wTIValue=7, const L3Cause& wCause = L3Cause(0x10))
		:L3CCMessage(wTIFlag,wTIValue),
		mCause(wCause)
	{}

	int MTI() const { return Disconnect; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody( const L3Frame &src, size_t &rp );
	size_t bodyLength() const { return mCause.lengthLV(); }
	void text(std::ostream&) const;

};



/** GSM 04.08 9.3.2 */
class L3CallConfirmed : public L3CCMessage {

private:

	bool mHaveCause;
	L3Cause	mCause;

public:

	L3CallConfirmed(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue),
		mHaveCause(false)
	{}

	int MTI() const { return CallConfirmed; }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody(const L3Frame &src, size_t &rp);
	size_t bodyLength() const;

	void text(std::ostream& os) const;

};



/** GSM 04.08 9.3.24 */
class L3StartDTMF : public L3CCMessage {

	private:

	L3KeypadFacility mKey;

	public:

	L3StartDTMF(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	const L3KeypadFacility& key() const { return mKey; }
	int MTI() const { return StartDTMF; }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody(const L3Frame &src, size_t &rp) { mKey.parseTV(0x2c,src,rp); }
	size_t bodyLength() const { return mKey.lengthTV(); }

	void text(std::ostream& os) const;
};


/** GSM 04.08 9.3.25 */
class L3StartDTMFAcknowledge : public L3CCMessage {

	private:

	L3KeypadFacility mKey;

	public:

	L3StartDTMFAcknowledge(unsigned wTIFlag, unsigned wTIValue, const L3KeypadFacility& wKey)
		:L3CCMessage(wTIFlag,wTIValue),
		mKey(wKey)
	{}

	int MTI() const { return StartDTMFAcknowledge; }
	void writeBody(L3Frame &dest, size_t &wp) const { mKey.writeTV(0x2C,dest,wp); }
	void parseBody(const L3Frame &, size_t&) { abort(); };
	size_t bodyLength() const { return mKey.lengthTV(); }

	void text(std::ostream& os) const;
};




/** GSM 04.08 9.3.26 */
class L3StartDTMFReject : public L3CCMessage {

	private:

	L3Cause mCause;

	public:

	/** Reject with default cause 0x3f, "service or option not available". */
	L3StartDTMFReject(unsigned wTIFlag=0, unsigned wTIValue=7, const L3Cause& wCause=L3Cause(0x3f))
		:L3CCMessage(wTIFlag,wTIValue),
		mCause(wCause)
	{}

	int MTI() const { return StartDTMFReject; }
	void parseBody(const L3Frame&, size_t&) { abort(); }
	void writeBody(L3Frame &src, size_t &rp) const { mCause.writeLV(src,rp); }
	size_t bodyLength() const { return mCause.lengthLV(); }

	void text(std::ostream& os) const;
};


/** GSM 04.08 9.3.29 */
class L3StopDTMF : public L3CCMessage {

	public:

	L3StopDTMF(unsigned wTIFlag=0, unsigned wTIValue=7)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	int MTI() const { return StopDTMF; }
	void writeBody(L3Frame&, size_t&) const { abort(); }
	void parseBody(const L3Frame &src, size_t &rp) { }
	size_t bodyLength() const { return 0; };
};


/** GSM 04.08 9.3.30 */
class L3StopDTMFAcknowledge : public L3CCMessage {

	public:

	L3StopDTMFAcknowledge(unsigned wTIFlag, unsigned wTIValue)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	int MTI() const { return StopDTMFAcknowledge; }
	void writeBody(L3Frame&, size_t&) const { }
	void parseBody(const L3Frame &src, size_t &rp) { abort(); };
	size_t bodyLength() const { return 0; }
};





/** GSM 04.08 9.3.17 */
class L3Progress : public L3CCMessage
{
	L3ProgressIndicator mProgress; 

	public:

	L3Progress(unsigned wTIFlag, unsigned wTIValue, const L3ProgressIndicator& wProgress)
		:L3CCMessage(wTIFlag,wTIValue),
		mProgress(wProgress)
	{}

	L3Progress(unsigned wTIFlag, unsigned wTIValue)
		:L3CCMessage(wTIFlag,wTIValue)
	{}

	int MTI() const { return Progress; }
	void writeBody( L3Frame &dest, size_t &wp ) const;
	void parseBody(const L3Frame &src, size_t &wp);
	size_t bodyLength() const;
	void text(std::ostream&) const;
};




/** GSM 04.08 9.3.6 */


};	// GSM


#endif
// vim: ts=4 sw=4
