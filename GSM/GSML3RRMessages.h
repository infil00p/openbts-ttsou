/**@file
	@brief L3 Radio Resource messages, GSM 04.08 9.1.
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



#ifndef GSML3RRMESSAGES_H
#define GSML3RRMESSAGES_H

#include "GSMCommon.h"
#include "GSML3Message.h"
#include "GSML3CommonElements.h"
#include "GSML3RRElements.h"

namespace GSM {


/**
	This a virtual class for L3 messages in the Radio Resource protocol.
	These messages are defined in GSM 04.08 9.1.
*/
class L3RRMessage : public L3Message {

	public:

	/** RR Message MTIs, GSM 04.08 Table 10.1/3. */
	enum MessageType {
		///@name System Information
		//@{
		SystemInformationType1=0x19,
		SystemInformationType2=0x1a,
		SystemInformationType2bis=0x02,
		SystemInformationType2ter=0x03,
		SystemInformationType3=0x1b,
		SystemInformationType4=0x1c,
		SystemInformationType5=0x1d,
		SystemInformationType5bis=0x05,
		SystemInformationType5ter=0x06,
		SystemInformationType6=0x1e,
		SystemInformationType7=0x1f,
		SystemInformationType8=0x18,
		SystemInformationType9=0x04,
		SystemInformationType13=0x00,
		SystemInformationType16=0x3d,
		SystemInformationType17=0x3e,
		//@}
		///@name Channel Management
		//@{
		AssignmentCommand=0x2e,
		AssignmentComplete=0x29,
		AssignmentFailure=0x2f,
		ChannelRelease=0x0d,
		ImmediateAssignment=0x3f,
		ImmediateAssignmentExtended=0x39,
		ImmediateAssignmentReject=0x3a,
		AdditionalAssignment=0x3b,
		//@}
		///@name Paging
		//@{
		PagingRequestType1=0x21,
		PagingRequestType2=0x22,
		PagingRequestType3=0x24,
		PagingResponse=0x27,
		//@}
		///@name Handover
		//@{
		HandoverCommand=0x2b,
		//@}
		///@name ciphering
		//@{
		CipheringModeCommand=0x35,
		//@}
		///@name miscellaneous
		//@{
		ChannelModeModify=0x10,
		ClassmarkChange=0x16,
		GPRSSuspensionRequest=0x34,
		RRStatus=0x12,
		//@}
		/// reporting
		MeasurementReport = 0x15,
		///@name special cases -- assigned >8-bit codes to avoid conflicts
		//@{
		SynchronizationChannelInformation=0x100,
		ChannelRequest=0x101
		//@}
	};


	L3RRMessage():L3Message() { } 
	
	/** Return the L3 protocol discriptor. */
	L3PD PD() const { return L3RadioResourcePD; }

	void text(std::ostream&) const;
};

std::ostream& operator<<(std::ostream& os, L3RRMessage::MessageType);



/**
	A Factory function to return a L3RRMessage of the specified MTI.
	Returns NULL if the MTI is not supported.
*/
L3RRMessage* L3RRFactory(L3RRMessage::MessageType MTI);

/**
	Parse a complete L3 radio resource message into its object type.
	@param source The L3 bits.
	@return A pointer to a new message or NULL on failure.
*/
L3RRMessage* parseL3RR(const L3Frame& source);


/** Paging Request Type 1, GSM 04.08 9.1.22 */
class L3PagingRequestType1 : public L3RRMessage {

	private:

	std::vector<L3MobileIdentity> mMobileIDs;


	public:

	L3PagingRequestType1()
		:L3RRMessage()
	{
		// The empty paging request is a single untyped mobile ID.
		mMobileIDs.push_back(L3MobileIdentity());
	}

	L3PagingRequestType1(const L3MobileIdentity& wId)
		:L3RRMessage()
	{ mMobileIDs.push_back(wId); }

	L3PagingRequestType1(const L3MobileIdentity& wId1, const L3MobileIdentity& wId2)
		:L3RRMessage()
	{
		mMobileIDs.push_back(wId1);
		mMobileIDs.push_back(wId2);
	}

	size_t bodyLength() const;

	int MTI() const { return PagingRequestType1; }

	void writeBody(L3Frame& dest, size_t& wp) const;

	void text(std::ostream&) const;
};




/** Paging Response, GSM 04.08 9.1.25 */
class L3PagingResponse : public L3RRMessage {

	private:

	L3MobileIdentity mMobileID;

	public:

	const L3MobileIdentity& mobileIdentity() const { return mMobileID; }

	int MTI() const { return PagingResponse; }
	size_t bodyLength() const;
	void parseBody(const L3Frame& source, size_t &rp);
	void text(std::ostream&) const;

};







/**
	System Information Message Type 1, GSM 04.08 9.1.31
	- Cell Channel Description 10.5.2.1b M V 16 
	- RACH Control Parameters 10.5.2.29 M V 3 
*/
class L3SystemInformationType1 : public L3RRMessage {

	private:

	L3FrequencyList mCellChannelDescription;
	L3RACHControlParameters mRACHControlParameters;

	public:

	L3SystemInformationType1():L3RRMessage() {}

	void RACHControlParameters(const L3RACHControlParameters& wRACHControlParameters)
		{ mRACHControlParameters = wRACHControlParameters; }

	void cellChannelDescription(const L3FrequencyList& wCellChannelDescription)
		{ mCellChannelDescription = wCellChannelDescription; }

	size_t bodyLength() const { return 19; }

	int MTI() const { return (int)SystemInformationType1; }

	void writeBody(L3Frame &dest, size_t &wp) const;

	void text(std::ostream&) const;
};






/**
	System Information Type 2, GSM 04.08 9.1.32.
	- BCCH Frequency List 10.5.2.22 M V 16 
	- NCC Permitted 10.5.2.27 M V 1 
	- RACH Control Parameter 10.5.2.29 M V 3 
*/
class L3SystemInformationType2 : public L3RRMessage {

	private:

	L3FrequencyList mBCCHFrequencyList;
	L3NCCPermitted mNCCPermitted;
	L3RACHControlParameters	mRACHControlParameters;

	public:

	L3SystemInformationType2():L3RRMessage() {}

	void BCCHFrequencyList(const L3FrequencyList& wBCCHFrequencyList)
		{ mBCCHFrequencyList = wBCCHFrequencyList; }

	void NCCPermitted(const L3NCCPermitted& wNCCPermitted)
		{ mNCCPermitted = wNCCPermitted; }

	void RACHControlParameters(const L3RACHControlParameters& wRACHControlParameters)
		{ mRACHControlParameters = wRACHControlParameters; }

	size_t bodyLength() const { return 20; }

	int MTI() const { return (int)SystemInformationType2; }

	void writeBody(L3Frame &dest, size_t &wp) const;

	void text(std::ostream&) const;
};





/**
	System Information Type 3, GSM 04.08 9.1.35
	- Cell Identity 10.5.1.1 M V 2 
	- Location Area Identification 10.5.1.3 M V 5 
	- Control Channel Description 10.5.2.11 M V 3 
	- Cell Options (BCCH) 10.5.2.3 M V 1 
	- Cell Selection Parameters 10.5.2.4 M V 2 
	- RACH Control Parameters 10.5.2.29 M V 3 
*/
class L3SystemInformationType3 : public L3RRMessage {

	private:

	L3CellIdentity mCI;
	L3LocationAreaIdentity mLAI;
	L3ControlChannelDescription mControlChannelDescription;
	L3CellOptionsBCCH mCellOptions;
	L3CellSelectionParameters mCellSelectionParameters;
	L3RACHControlParameters mRACHControlParameters;

	public:

	L3SystemInformationType3():L3RRMessage() {}

	void CI(const L3CellIdentity& wCI) { mCI = wCI; }

	void LAI(const L3LocationAreaIdentity& wLAI) { mLAI = wLAI; }

	void controlChannelDescription(const L3ControlChannelDescription& wControlChannelDescription)
		{ mControlChannelDescription = wControlChannelDescription; }

	void cellOptions(const L3CellOptionsBCCH& wCellOptions)
		{ mCellOptions = wCellOptions; }

	void cellSelectionParameters (const L3CellSelectionParameters& wCellSelectionParameters)
		{ mCellSelectionParameters = wCellSelectionParameters; }

	void RACHControlParameters(const L3RACHControlParameters& wRACHControlParameters)
		{ mRACHControlParameters = wRACHControlParameters; }

	size_t bodyLength() const { return 16; }

	int MTI() const
		{ return (int)SystemInformationType3; }

	void writeBody(L3Frame &dest, size_t &wp) const;

	void text(std::ostream&) const;
};






/**
	System Information Type 4, GSM 04.08 9.1.36
	- Location Area Identification 10.5.1.3 M V 5 
	- Cell Selection Parameters 10.5.2.4 M V 2 
	- RACH Control Parameters 10.5.2.29 M V 3 
*/
class L3SystemInformationType4 : public L3RRMessage {

	private:

	L3LocationAreaIdentity mLAI;
	L3CellSelectionParameters mCellSelectionParameters;
	L3RACHControlParameters mRACHControlParameters;

	public:

	L3SystemInformationType4():L3RRMessage() {}

	void LAI(const L3LocationAreaIdentity& wLAI) { mLAI = wLAI; }

	void cellSelectionParameters (const L3CellSelectionParameters& wCellSelectionParameters)
		{ mCellSelectionParameters = wCellSelectionParameters; }

	void RACHControlParameters(const L3RACHControlParameters& wRACHControlParameters)
		{ mRACHControlParameters = wRACHControlParameters; }

	size_t bodyLength() const
	{
		return mLAI.lengthV() +
		mCellSelectionParameters.lengthV() + mRACHControlParameters.lengthV();
	}

	int MTI() const { return (int)SystemInformationType4; }

	void writeBody(L3Frame &dest, size_t &wp) const;

	void text(std::ostream&) const;
};




/**
	System Information Type 5, GSM 04.08 9.1.37
	- BCCH Frequency List 10.5.2.22 M V 16 
*/
class L3SystemInformationType5 : public L3RRMessage {

	private:

	L3FrequencyList mBCCHFrequencyList;

	public:

	L3SystemInformationType5():L3RRMessage() { }

	void BCCHFrequencyList(const L3FrequencyList& wBCCHFrequencyList)
		{ mBCCHFrequencyList = wBCCHFrequencyList; }

	size_t bodyLength() const { return 16; }

	int MTI() const
		 { return (int)SystemInformationType5; }

	void writeBody(L3Frame &dest, size_t &wp) const;
	void text(std::ostream&) const;
};





/**
	System Information Type 6, GSM 04.08 9.1.40
	- Cell Identity 10.5.1.11 M V 2 
	- Location Area Identification 10.5.1.3 M V 5 
	- Cell Options (SACCH) 10.5.2.3 M V 1 
	- NCC Permitted 10.5.2.27 M V 1 
*/
class L3SystemInformationType6 : public L3RRMessage {

	private:

	L3CellIdentity mCI;
	L3LocationAreaIdentity mLAI;
	L3CellOptionsSACCH mCellOptions;
	L3NCCPermitted mNCCPermitted;

	public:

	L3SystemInformationType6():L3RRMessage() {}

	void CI(const L3CellIdentity& wCI) { mCI = wCI; }

	void LAI(const L3LocationAreaIdentity& wLAI) { mLAI = wLAI; }

	void cellOptions(const L3CellOptionsSACCH& wCellOptions)
		{ mCellOptions = wCellOptions; }

	void NCCPermitted(const L3NCCPermitted& wNCCPermitted)
		{ mNCCPermitted = wNCCPermitted; }

	size_t bodyLength() const { return 9; }

	int MTI() const { return (int)SystemInformationType6; }

	void writeBody(L3Frame &dest, size_t &wp) const;

	void text(std::ostream&) const;
};




/** Immediate Assignment, GSM 04.08 9.1.18 */
class L3ImmediateAssignment : public L3RRMessage {

private:

	L3PageMode mPageMode;
	L3DedicatedModeOrTBF mDedicatedModeOrTBF;
	L3RequestReference mRequestReference;
	L3ChannelDescription mChannelDescription;  
	L3TimingAdvance mTimingAdvance;

public:


	L3ImmediateAssignment(
				const L3RequestReference& wRequestReference,
				const L3ChannelDescription& wChannelDescription)
		:L3RRMessage(),
		mRequestReference(wRequestReference),
		mChannelDescription(wChannelDescription)
	{}

	void writeBody(L3Frame &dest, size_t &wp) const;
	int MTI() const { return (int)ImmediateAssignment; }
	size_t bodyLength() const { return 9; }

	void text(std::ostream&) const;

};



/** Immediate Assignment Reject, GSM 04.08 9.1.20 */
class L3ImmediateAssignmentReject : public L3RRMessage {

private:

	L3PageMode mPageMode;
	std::vector<L3RequestReference> mRequestReference;
	L3WaitIndication mWaitIndication;			///< All entries get the same wait indication.

public:


	L3ImmediateAssignmentReject(const L3RequestReference& wRequestReference, unsigned seconds=T3122ms/1000)
		:L3RRMessage(),
		mWaitIndication(seconds)
	{ mRequestReference.push_back(wRequestReference); }

	void writeBody(L3Frame &dest, size_t &wp) const;
	int MTI() const { return (int)ImmediateAssignmentReject; }
	size_t bodyLength() const { return 17; }

	void text(std::ostream&) const;

};






/** GSM 04.08 9.1.7 */
class L3ChannelRelease : public L3RRMessage {

private:

	L3RRCause mRRCause;

public:
	
	/** The default cause is 0x0, "normal event". */
	L3ChannelRelease(const L3RRCause& cause = L3RRCause(0x0))
		:L3RRMessage(),mRRCause(cause)
	{}

	void writeBody( L3Frame &dest, size_t &wp ) const; 

	int MTI() const { return (int) ChannelRelease; }
	size_t bodyLength() const { return mRRCause.lengthV(); }

	void text(std::ostream&) const;
};




/**
	GSM 04.08 9.1.8
	The channel request message is special because the timestamp
	is implied by the receive time but not actually present in
	the message.
	This messages has no parse or write methods, but is used to
	transfer information from L1 to the control layer.
*/
class L3ChannelRequest : public L3RRMessage {

	private:

	unsigned mRA;		///< request reference
	GSM::Time mTime;		///< receive timestamp

	public:

	L3ChannelRequest(unsigned wRA, const GSM::Time& wTime)
		:L3RRMessage(),
		mRA(wRA), mTime(wTime)
	{}

	/**@name Accessors. */
	//@{
	unsigned RA() const { return mRA; }
	const GSM::Time& time() const { return mTime; }
	//@}

	int MTI() const { return (int)ChannelRequest; }
	size_t lengthV() const { return 0; }

	void text(std::ostream&) const;
};





/** GSM 04.08 9.1.2 */
class L3AssignmentCommand : public L3RRMessage {

private:

	L3ChannelDescription mChannelDescription;
	L3PowerCommand	mPowerCommand;
	
	bool mHaveMode1;
	L3ChannelMode mMode1;	

public:

	L3AssignmentCommand(const L3ChannelDescription& wChannelDescription,
			const L3ChannelMode& wMode1 )
		:L3RRMessage(),
		mChannelDescription(wChannelDescription),
		mHaveMode1(true),mMode1(wMode1)
	{}

	void writeBody( L3Frame &dest, size_t &wp ) const; 

	int MTI() const { return (int) AssignmentCommand; }
	size_t bodyLength() const;

	void text(std::ostream&) const;
};


/** GSM 04.08 9.1.3 */
class L3AssignmentComplete : public L3RRMessage {

	private:

	L3RRCause mCause;

	public:

	///@name Accessors.
	//@{
	const L3RRCause& cause() const { return mCause; }
	//@}

	void parseBody( const L3Frame &src, size_t &rp );
	int MTI() const { return (int) AssignmentComplete; }
	size_t bodyLength() const { return 1; }

	void text(std::ostream&) const;

};


/** GSM 04.08 9.1.3 */
class L3AssignmentFailure : public L3RRMessage {

	private:

	L3RRCause mCause;

	public:

	///@name Accessors.
	//@{
	const L3RRCause& cause() const { return mCause; }
	//@}

	void parseBody( const L3Frame &src, size_t &rp );

	int MTI() const { return (int) AssignmentFailure; }
	size_t bodyLength() const { return 1; }

	void text(std::ostream&) const;

};




/** GSM 04.08 9.1.29 */
class L3RRStatus : public L3RRMessage {

	private:

	L3RRCause mCause;

	public:

	///@name Accessors.
	//@{
	const L3RRCause& cause() const { return mCause; }
	//@}

	void parseBody( const L3Frame &src, size_t &rp );

	int MTI() const { return (int) RRStatus; }
	size_t bodyLength() const { return 1; }

	void text(std::ostream&) const;

};




}; // GSM



#endif
// vim: ts=4 sw=4
