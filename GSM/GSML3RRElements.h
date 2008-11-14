/**@file
	@brief Elements for Radio Resource messsages, GSM 04.08 10.5.2.
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


#ifndef GSML3RRELEMENTS_H
#define GSML3RRELEMENTS_H

#include <vector>
#include "GSML3Message.h"


namespace GSM {


/** Cell Options (BCCH), GSM 04.08 10.5.2.3 */
class L3CellOptionsBCCH : public L3ProtocolElement {

	private:

	unsigned mPWRC;					///< 1 -> downlink power control may be used
	unsigned mDTX;					///< discontinuous transmission state
	unsigned mRADIO_LINK_TIMEOUT;	///< timeout to declare dead phy link

	public:

	/** Sets defaults for no use of DTX or downlink power control. */
	L3CellOptionsBCCH():L3ProtocolElement()
	{
		// FIXME -- These should be drawn from gBTS.
		mPWRC=0;					// no power control
		mDTX=2;						// no DTX
		mRADIO_LINK_TIMEOUT=15;		// 64-frame timeout
	}

	size_t lengthV() const { return 1; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;
};




/** Cell Options (SACCH), GSM 04.08 10.5.2.3a */
class L3CellOptionsSACCH : public L3ProtocolElement {

	private:

	unsigned mPWRC;					///< 1 -> downlink power control may be used
	unsigned mDTX;					///< discontinuous transmission state
	unsigned mRADIO_LINK_TIMEOUT;	///< timeout to declare dead phy link

	public:

	/** Sets defaults for no use of DTX or downlink power control. */
	L3CellOptionsSACCH():L3ProtocolElement()
	{
		/* reasonable defaults */
		mPWRC=0;					// no power control
		mDTX=2;						// no DTX
		// FIXME -- Should RADIO_LINK_TIMEOUT be linked to T3109?
		mRADIO_LINK_TIMEOUT=15;		// 64-frame timeout
	}

	size_t lengthV() const { return 1; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;
	
};





/** Cell Selection Parameters, GSM 04.08 10.5.2.4 */
class L3CellSelectionParameters : public L3ProtocolElement {

	private:

	unsigned mCELL_RESELECT_HYSTERESIS;
	unsigned mMS_TXPWR_MAX_CCH;
	unsigned mACS;
	unsigned mNECI;
	unsigned mRXLEV_ACCESS_MIN;

	public:

	/** Sets defaults to reduce gratuitous handovers. */
	L3CellSelectionParameters()
		:L3ProtocolElement()
	{
		mCELL_RESELECT_HYSTERESIS=6;		// 8 dB reselect
		mMS_TXPWR_MAX_CCH=0;				// a high power level in all bands
		mACS=0;								// no additional relesect parameters
		mNECI=0;							// new establishment causes not supported
		mRXLEV_ACCESS_MIN=0;				// lowest allowed access level
	}

	size_t lengthV() const { return 2; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;

};




/** Control Channel Description, GSM 04.08 10.5.2.11 */
class L3ControlChannelDescription : public L3ProtocolElement {

	private:

	unsigned mATT;				///< 1 -> IMSI attach/detach
	unsigned mBS_AG_BLKS_RES;	///< access grant channel reservation
	unsigned mCCCH_CONF;			///< channel combination for CCCH
	unsigned mBS_PA_MFRMS;		///< paging channel configuration
	unsigned mT3212;				///< periodic updating timeout

	public:

	/** Sets reasonable defaults for a single-ARFCN system .*/
	L3ControlChannelDescription():L3ProtocolElement()
	{
		//FIXME These values need to be tied to some global configuration.
		mATT=1;						// IMSI attach/detach
		mBS_AG_BLKS_RES=2;			// reserve 2 CCCHs for access grant
		mCCCH_CONF=1;				// C-V beacon
		mBS_PA_MFRMS=0;				// minimum PCH spacing
		mT3212=T3212ms/360000;		// registration period, in 6-min increments
	}

	size_t lengthV() const { return 3; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;

};



/**
	A generic frequency list element base class, GSM 04.08 10.5.2.13.
	This implementation supports only the "variable bit map" format
	(GSM 04.08 10.5.2.13.7).
*/
class L3FrequencyList : public L3ProtocolElement {

	protected:

	std::vector<unsigned> mARFCNs;		///< ARFCN list to encode/decode

	public:

	/** Default constructor creates an empty list. */
	L3FrequencyList():L3ProtocolElement() {}

	size_t lengthV() const { return 16; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;

	private:

	/**@name ARFCN set browsing. */
	//@{
	/** Return minimum-numbered ARFCN. */
	unsigned base() const;

	/** Return numeric spread of ARFNs. */
	unsigned spread() const;

	/** Return true if a given ARFCN is in the list. */
	bool contains(unsigned wARFCN) const;
	//@}
};



/**
	Cell Channel Description, GSM 04.08 10.5.2.1b.
	This element is used to provide the Cell Allocation
	for frequency hopping configurations.
	It lists the ARFCNs available for hopping and 
	normally lists all of the ARFCNs for the system.
	It is mandatory, even if you don't use hopping.
*/
class L3CellChannelDescription : public L3FrequencyList {

	public:

	L3CellChannelDescription()
		:L3FrequencyList()
	{}


	void writeV(L3Frame& dest, size_t &wp) const;

};





/**
	Neighbor Cells Description, GSM 04.08 10.5.2.22
	(A kind of frequency list.)
	This element describes neighboring cells that may be 
	candidates for handovers.
*/
class L3NeighborCellsDescription : public L3FrequencyList {

	public:

	L3NeighborCellsDescription()
		:L3FrequencyList()
	{}

	void writeV(L3Frame& dest, size_t &wp) const;

	void text(std::ostream&) const;

};




/** NCC Permitted, GSM 04.08 10.5.2.27 */
class L3NCCPermitted : public L3ProtocolElement {

	private:

	unsigned mPermitted;			///< NCC allowance mask (NCCs 0-7)

	public:

	/** Default allows measurements for all 8 NCCs. */
	L3NCCPermitted(unsigned wPermitted=0x0ff)
		:L3ProtocolElement(),
		mPermitted(wPermitted)
	{ }

	size_t lengthV() const { return 1; }

	void writeV(L3Frame& dest, size_t &wp) const;

	void text(std::ostream&) const;

};




/** RACH Control Parameters GSM 04.08 10.5.2.29 */
class L3RACHControlParameters : public L3ProtocolElement {

	private:

	unsigned mMaxRetrans;		///< code for 1-7 RACH retransmission attempts
	unsigned mTxInteger;		///< code for 3-50 slots to spread transmission
	unsigned mCellBarAccess;	///< if true, phones cannot camp
	unsigned mRE;				///< if true, call reestablishment is not allowed
	uint16_t mAC;				///< mask of barring flags for the 16 access classes

	public:

	// FIXME -- These should be drawn from gBTS.
	/** Default constructor parameters allows all access. */
	L3RACHControlParameters(
			unsigned wMaxRetrans=0x03, unsigned wTxInteger=0x0e,
			unsigned wCellBarAccess=0, unsigned wRE=0,
			uint16_t wAC=0x0)
		:L3ProtocolElement(),
		mMaxRetrans(wMaxRetrans),mTxInteger(wTxInteger),
		mCellBarAccess(wCellBarAccess),mRE(wRE),
		mAC(wAC)
	{}

	size_t lengthV() const { return 3; }
	void writeV(L3Frame& dest, size_t &wp) const;

	void text(std::ostream&) const;

};





/** PageMode, GSM 04.08 10.5.2.26 */
class L3PageMode : public L3ProtocolElement
{


	unsigned mPageMode;

public:

	/** Default mode is "normal paging". */
	L3PageMode(unsigned wPageMode=0)
		:L3ProtocolElement(),
		mPageMode(wPageMode)
	{}

	void writeV( L3Frame& dest, size_t &wp ) const;
	void parseV( const L3Frame &src, size_t &rp );
	size_t lengthV() const { return 1; }

	void text(std::ostream&) const;

};



/** DedicatedModeOrTBF, GSM 04.08 10.5.2.25b */
class L3DedicatedModeOrTBF : public L3ProtocolElement {

	unsigned mDownlink;		///< Indicates the IA reset octets contain additional information.
	unsigned mTMA;			///< This is part of a 2-message assignment.
	unsigned mDMOrTBF;		///< Dedicated link (circuit-switched) or temporary block flow (GPRS/pakcet).

	
public:
	
	L3DedicatedModeOrTBF()
		:L3ProtocolElement(),
		mDownlink(0), mTMA(0), mDMOrTBF(0)
	{}

	void writeV(L3Frame &dest, size_t &wp ) const;
	size_t lengthV() const { return 1; }

	void text(std::ostream&) const;

};



/** ChannelDescription, GSM 04.08 10.5.2.5 */
class L3ChannelDescription : public L3ProtocolElement {


//                  Channel Description Format.
//          7      6      5      4      3     2      1      0
//	  [ ChannelTypeTDMAOffset[4:0]      ][   TN[2:0]		]  Octect 1
//    [         TSC       ][ H=0 ][ SPARE(0,0)][ ARFCN[9:8] ]  Octect 2
//    [                    [ H=1 ][  MAIO[5:2]              ]  Octect 2
//    [                ARFCN[7:0]                           ]  Octect 3 H=0
//    [ MAIO[1:0]  ][        HSN[5:0]                       ]  Octect 3 H=1
//

	// Octet 2.
	TypeAndOffset mTypeAndOffset; // 5 bit
	unsigned mTN; 		//3 bit 

	// Octet 3 & 4.
	unsigned mTSC; 		// 3 bit
	unsigned mHFlag; 	// 1 bit
	unsigned mARFCN;	// 10 bit overflows
	unsigned mMAIO;		// 6 bit overflows
	unsigned mHSN;		// 6 bit
	
public:

	/** Non-hopping initializer. */
	L3ChannelDescription(TypeAndOffset wTypeAndOffset, unsigned wTN,
			unsigned wTSC, unsigned wARFCN)
		:mTypeAndOffset(wTypeAndOffset),mTN(wTN),
		mTSC(wTSC),
		mHFlag(0),
		mARFCN(wARFCN),
		mMAIO(0),mHSN(0)
	{ }
	

	void writeV( L3Frame &dest, size_t &wp ) const;

	size_t lengthV() const  { return 3; }

	void text(std::ostream&) const;

};







/** RequestReference, GSM 04.08 10.5.2.30 */
class L3RequestReference : public L3ProtocolElement
{

//                  Request Reference Format.
//          7      6      5      4      3     2      1      0
//    [                 RequestReference [7:0]              ]  Octet 2
//    [         T1[4:0]                 ][   T3[5:3]        ]  Octet 3
//    [       T3[2:0]     ][            T2[4:0]             ]  Octet 4

	unsigned mRA;			///< random tag from original RACH burst

	/**@name Timestamp of the corresponing RACH burst. */
	//@{
	unsigned mT1p;		///< T1 mod 32
	unsigned mT2;
	unsigned mT3;
	//@}

public:

	L3RequestReference() {}

	L3RequestReference(unsigned wRA, const GSM::Time& when)
		:mRA(wRA),
		mT1p(when.T1()%32),mT2(when.T2()),mT3(when.T3())
	{}

	void writeV(L3Frame &, size_t &wp ) const;
	size_t lengthV() const { return 3; }

	void text(std::ostream&) const;

};



/** Timing Advance, GSM 04.08 10.5.2.40 */
class L3TimingAdvance : public L3ProtocolElement
{
//							TimingAdvance
//          7      6      5      4      3     2      1      0
//    [    spare(0,0)     ][      TimingAdvance [5:0]              ]  Octet 1

	unsigned mTimingAdvance;
	
public:

	L3TimingAdvance(unsigned wTimingAdvance=0)
		:L3ProtocolElement(),
		mTimingAdvance(wTimingAdvance)
	{}
	
	void writeV( L3Frame &dest, size_t &wp ) const;

	size_t lengthV() const { return 1; }

	void text(std::ostream&) const;

};




/** GSM 04.08 10.5.2.31 */
class L3RRCause : public L3ProtocolElement
{
	int mCauseValue;

	public:

	L3RRCause(int wValue=0)
		:L3ProtocolElement()
	{ mCauseValue=wValue; }

	int causeValue() const { return mCauseValue; }

	void writeV( L3Frame &dest, size_t &wp ) const;
	void parseV( const L3Frame &src, size_t &rp );
	size_t lengthV() const { return 1; }

	void text(std::ostream&) const;

};





/** GSM 04.08 10.5.2.28 */
class L3PowerCommand : public L3ProtocolElement
{
	unsigned mCommand;

public:

	L3PowerCommand(unsigned wCommand=0)
		:L3ProtocolElement(),
		mCommand(wCommand)
	{}

	void writeV( L3Frame &dest, size_t &wp ) const;
	size_t lengthV() const { return 1; }

	void text(std::ostream&) const;

};



/** GSM 04.08 10.5.2.6 */
class L3ChannelMode : public L3ProtocolElement {

public:

	enum Mode 
	{
		SignallingOnly=0,
		SpeechV1=1,
		SpeechV2=2,
		SpeechV3=3
	};

private:

	Mode mMode;

public:

	L3ChannelMode(Mode wMode=SignallingOnly)
		:L3ProtocolElement(), 
		mMode(wMode)
	{}

	void writeV(L3Frame& dest, size_t &wp) const;
	size_t lengthV() const { return 1; }
	void text(std::ostream&) const;

};

std::ostream& operator<<(std::ostream&, L3ChannelMode::Mode);




/** GSM 04.08 10.5.2.43 */
class L3WaitIndication : public L3ProtocolElement {

	private:

	unsigned mValue;		///< T3122 or T3142 value in seconds

	public:

	L3WaitIndication(unsigned seconds=T3122ms/1000)
		:L3ProtocolElement(),
		mValue(seconds)
	{}

	void writeV(L3Frame& dest, size_t &wp) const
		{ dest.writeField(wp,mValue,8); }

	size_t lengthV() const
		{ return 1; }

	void text(std::ostream& os) const
		{ os << mValue; }

};




} // GSM


#endif



// vim: ts=4 sw=4