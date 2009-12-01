/**@file
	@brief Radio Resource messages, GSM 04.08 9.1.
*/

/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
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


#include "GSML3RRElements.h"

#include <Logger.h>



using namespace std;
using namespace GSM;




void L3CellOptionsBCCH::writeV(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,0,1);
	dest.writeField(wp,mPWRC,1);
	dest.writeField(wp,mDTX,2);
	dest.writeField(wp,mRADIO_LINK_TIMEOUT,4);
}



void L3CellOptionsBCCH::text(ostream& os) const
{
	os << "PWRC=" << mPWRC;
	os << " DTX=" << mDTX;
	os << " RADIO_LINK_TIMEOUT=" << mRADIO_LINK_TIMEOUT;
}



void L3CellOptionsSACCH::writeV(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,(mDTX>>2)&0x01,1);
	dest.writeField(wp,mPWRC,1);
	dest.writeField(wp,mDTX&0x03,2);
	dest.writeField(wp,mRADIO_LINK_TIMEOUT,4);
}



void L3CellOptionsSACCH::text(ostream& os) const
{
	os << "PWRC=" << mPWRC;
	os << " DTX=" << mDTX;
	os << " RADIO_LINK_TIMEOUT=" << mRADIO_LINK_TIMEOUT;
}



void L3CellSelectionParameters::writeV(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,mCELL_RESELECT_HYSTERESIS,3);
	dest.writeField(wp,mMS_TXPWR_MAX_CCH,5);
	dest.writeField(wp,mACS,1);
	dest.writeField(wp,mNECI,1);
	dest.writeField(wp,mRXLEV_ACCESS_MIN,6);
}




void L3CellSelectionParameters::text(ostream& os) const
{
	os << "CELL-RESELECT-HYSTERESIS=" << mCELL_RESELECT_HYSTERESIS;
	os << " MS-TXPWR-MAX-CCH=" << mMS_TXPWR_MAX_CCH;
	os << " ACS=" << mACS;
	os << " NECI=" << mNECI;
	os << " RXLEV-ACCESS-MIN=" << mRXLEV_ACCESS_MIN;
}







void L3ControlChannelDescription::writeV(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,0,1);
	dest.writeField(wp,mATT,1);
	dest.writeField(wp,mBS_AG_BLKS_RES,3);
	dest.writeField(wp,mCCCH_CONF,3);
	dest.writeField(wp,0,5);
	dest.writeField(wp,mBS_PA_MFRMS,3);
	dest.writeField(wp,mT3212,8);
}



void L3ControlChannelDescription::text(ostream& os) const
{
	os << "ATT=" << mATT;
	os << " BS_AG_BLKS_RES=" << mBS_AG_BLKS_RES;
	os << " CCCH_CONF=" << mCCCH_CONF;
	os << " BS_PA_MFRMS=" << mBS_PA_MFRMS;
	os << " T3212=" << mT3212;
}


bool L3FrequencyList::contains(unsigned wARFCN) const
{
	for (unsigned i=0; i<mARFCNs.size(); i++) {
		if (mARFCNs[i]==wARFCN) return true;
	}
	return false;
}


unsigned L3FrequencyList::base() const
{
	if (mARFCNs.size()==0) return 0;
	unsigned retVal = mARFCNs[0];
	for (unsigned i=1; i<mARFCNs.size(); i++) {
		unsigned thisVal = mARFCNs[i];
		if (thisVal<retVal) retVal=thisVal;
	}
	return retVal;
}


unsigned L3FrequencyList::spread() const
{
	if (mARFCNs.size()==0) return 0;
	unsigned max = mARFCNs[0];
	for (unsigned i=0; i<mARFCNs.size(); i++) {
		if (mARFCNs[i]>max) max=mARFCNs[i];
	}
	return max - base();
}


void L3FrequencyList::writeV(L3Frame& dest, size_t &wp) const
{
	// If this were used as Frequency List, it had to be coded
	// as the variable bit map format, GSM 04.08 10.5.2.13.7.
	// But it is used as Cell Channel Description and is coded
	// as the variable bit map format, GSM 04.08 10.5.2.1b.7.
	// Difference is in abscence of Length field.
	
	// The header occupies first 7 most significant bits of
	// the first V-part octet and should be 1000111b=0x47 for
	// the variable length bitmap.
	dest.writeField(wp,0x47,7);

	// For some formats, some of the first 7 bits are not spare.
	// For those formats, the caller will need to write those
	// bits after calling this method.

	// base ARFCN
	unsigned baseARFCN = base();
	dest.writeField(wp,baseARFCN,10);
	// bit map
	unsigned delta = spread();
	unsigned numBits = 8*lengthV() - 17;
	if (numBits<delta) LOG(ALARM) << "L3FrequencyList cannot encode full ARFCN set";
	for (unsigned i=0; i<numBits; i++) {
		unsigned thisARFCN = baseARFCN + 1 + i;
		if (contains(thisARFCN)) dest.writeField(wp,1,1);
		else dest.writeField(wp,0,1);
	}
}



void L3FrequencyList::text(ostream& os) const
{
	int size = mARFCNs.size();
	for (int i=0; i<size; i++) {
		os << mARFCNs[i] << " ";
	}
}




void L3CellChannelDescription::writeV(L3Frame& dest, size_t& wp) const
{
	L3FrequencyList::writeV(dest,wp);
}



void L3NeighborCellsDescription::writeV(L3Frame& dest, size_t& wp) const
{
	L3FrequencyList::writeV(dest,wp);
	// EXT-IND and BA-IND bits.
	dest.fillField(2,0,3);
}



void L3NeighborCellsDescription::text(ostream& os) const
{
	os << "EXT-IND=0 BA-IND=0 ";
	os << " ARFCNs=(";
	L3FrequencyList::text(os);
	os << ")";
}




void L3NCCPermitted::writeV(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,mPermitted,8);
}



void L3NCCPermitted::text(ostream& os) const
{
	os << hex << "0x" << mPermitted << dec;
}




void L3RACHControlParameters::writeV(L3Frame& dest, size_t &wp) const
{
	// GMS 04.08 10.5.2.29
	dest.writeField(wp, mMaxRetrans, 2);
	dest.writeField(wp, mTxInteger, 4);
	dest.writeField(wp, mCellBarAccess, 1);
	dest.writeField(wp, mRE, 1);
	dest.writeField(wp, mAC, 16);
}


void L3RACHControlParameters::text(ostream& os) const
{
	os << "maxRetrans=" << mMaxRetrans;
	os << " txInteger=" << mTxInteger;
	os << " cellBarAccess=" << mCellBarAccess;
	os << " RE=" << mRE;
	os << hex << " AC=0x" << mAC << dec;
}



void L3PageMode::writeV(L3Frame& dest, size_t &wp) const
{
	// PageMode is 1/2 octet. Spare[3:2], PM[1:0]
	dest.writeField(wp, 0x00, 2);
	dest.writeField(wp, mPageMode, 2);
}

void L3PageMode::parseV( const L3Frame &src, size_t &rp)
{
	// Read out spare bits.
	rp += 2;
	// Read out PageMode.
	mPageMode = src.readField(rp, 2);
} 



void L3PageMode::text(ostream& os) const
{
	os << mPageMode;
}


void L3DedicatedModeOrTBF::writeV( L3Frame& dest, size_t &wp )const
{
	// 1/2 Octet. 
	dest.writeField(wp, 0, 1);
	dest.writeField(wp, mTMA, 1);
	dest.writeField(wp, mDownlink, 1);
	dest.writeField(wp, mDMOrTBF, 1);
}


void L3DedicatedModeOrTBF::text(ostream& os) const
{
	os << "TMA=" << mTMA;
	os << " Downlink=" << mDownlink;
	os << " DMOrTBF=" << mDMOrTBF;
}


void L3ChannelDescription::writeV( L3Frame &dest, size_t &wp ) const 
{
	// GSM 04.08 10.5.2.5
// 					Channel Description Format (non-hopping)
// 		 	7      6      5      4      3     2      1      0
//	  [         TSC       ][ H=0 ][ SPARE(0,0)][ ARFCN[9:8] ]  Octet 3
//	  [                ARFCN[7:0]                           ]  Octet 4 H=0
//

	// HACK -- Hard code for non-hopping.
	assert(mHFlag==0);
	dest.writeField(wp,mTypeAndOffset,5);
	dest.writeField(wp,mTN,3);
	dest.writeField(wp,mTSC,3);
	dest.writeField(wp,0,3);				// H=0 + 2 spares
	dest.writeField(wp,mARFCN,10);
}



void L3ChannelDescription::parseV(const L3Frame& src, size_t &rp)
{
	// GSM 04.08 10.5.2.5
	mTypeAndOffset = (TypeAndOffset)src.readField(rp,5);
	mTN = src.readField(rp,3);
	mTSC = src.readField(rp,3);
	mHFlag = src.readField(rp,1);
	if (mHFlag) {
		mMAIO = src.readField(rp,6);
		mHSN = src.readField(rp,6);
	} else {
		rp += 2;	// skip 2 spare bits
		mARFCN = src.readField(rp,10);
	}
}


void L3ChannelDescription::text(std::ostream& os) const
{

	os << "typeAndOffset=" << mTypeAndOffset;
	os << " TN=" << mTN;
	os << " TSC=" << mTSC;
	os << " ARFCN=" << mARFCN;	
}



void L3RequestReference::writeV( L3Frame &dest, size_t &wp ) const 
{


// 					Request Reference Format.
// 		 	7      6      5      4      3     2      1      0
//	  [      			RequestReference [7:0]   			]  Octet 2
//	  [			T1[4:0]                 ][   T3[5:3]        ]  Octet 3
//	  [       T3[2:0]     ][            T2[4:0]             ]  Octet 4

	dest.writeField(wp, mRA, 8);
	dest.writeField(wp, mT1p, 5);
	dest.writeField(wp, mT3, 6);
	dest.writeField(wp, mT2, 5);
}


void L3RequestReference::text(ostream& os) const
{
	os << "RA=" << mRA;	
	os << " T1'=" << mT1p;
	os << " T2=" << mT2;
	os << " T3=" << mT3;	
}




void L3TimingAdvance::writeV( L3Frame &dest, size_t &wp ) const
{
	dest.writeField(wp, 0x00, 2);
	dest.writeField(wp, mTimingAdvance, 6);
}

void L3TimingAdvance::text(ostream& os) const
{
    os << mTimingAdvance;
}



void L3RRCause::writeV( L3Frame &dest, size_t &wp ) const
{
	dest.writeField(wp, mCauseValue, 8);	
}

void L3RRCause::parseV( const L3Frame &src, size_t &rp )
{
	mCauseValue = src.readField(rp, 8);
}

void L3RRCause::text(ostream& os) const
{
	os << "0x" << hex << mCauseValue << dec;
}




void L3PowerCommand::writeV( L3Frame &dest, size_t &wp )const
{
	dest.writeField(wp, 0, 3);
	dest.writeField(wp, mCommand, 5);
}


void L3PowerCommand::text(ostream& os) const
{
	os << mCommand;
}





void L3ChannelMode::writeV( L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp, mMode, 8);
}

void L3ChannelMode::parseV(const L3Frame& src, size_t& rp)
{
	mMode = (Mode)src.readField(rp,8);
}



ostream& GSM::operator<<(ostream& os, L3ChannelMode::Mode mode)
{
	switch (mode) {
		case L3ChannelMode::SignallingOnly: os << "signalling"; break;
		case L3ChannelMode::SpeechV1: os << "speech1"; break;
		case L3ChannelMode::SpeechV2: os << "speech2"; break;
		case L3ChannelMode::SpeechV3: os << "speech3"; break;
		default: os << "?" << (int)mode << "?";
	}
	return os;
}

void L3ChannelMode::text(ostream& os) const
{
	os << mMode;
}


// vim: ts=4 sw=4
