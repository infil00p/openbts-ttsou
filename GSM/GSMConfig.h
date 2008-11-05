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
Harvind S. Samra, hssamra@kestrelsp.com
*/


/*
	Compilation flags:
	NOL3	Comile with minimal references to L3
*/

#ifndef GSMCONFIG_H
#define GSMCONFIG_H

#include <vector>

#include "ControlCommon.h"

#include "GSML3RRElements.h"
#include "GSML3CommonElements.h"

#ifndef NOL3
#include "GSML3RRMessages.h"
#endif



namespace GSM {


class CCCHLogicalChannel;
class SDCCHLogicalChannel;
class TCHFACCHLogicalChannel;

class CCCHList : public std::vector<CCCHLogicalChannel*> {};
class SDCCHList : public std::vector<SDCCHLogicalChannel*> {};
class TCHList : public std::vector<TCHFACCHLogicalChannel*> {};

/**
	This object carries the top-level GSM air interface configuration.
	It serves as a central clearinghouse to get access to everything else in the GSM code.
*/
class GSMConfig {

	private:

	/** The paging mechanism is built-in. */
	Control::Pager mPager;

	mutable Mutex mLock;						///< multithread access control

	/**@name Groups of CCCH subchannels -- may intersect. */
	//@{
	CCCHList mAGCHPool;		///< access grant CCCH subchannels
	CCCHList mPCHPool;		///< paging CCCH subchannels
	//@}

	/**@name Allocatable channel pools. */
	//@{
	SDCCHList mSDCCHPool;
	TCHList mTCHPool;
	//@}

	/**@name BSIC. */
	//@{
	unsigned mNCC;		///< network color code
	unsigned mBCC;		///< basestation color code
	//@}

	GSMBand mBand;		///< BTS operating band

	Clock mClock;		///< local copy of BTS master clock

	/**@name System Information messages and related objects. */
	//@{
	/**@name Beacon parameters that appear on the BCCH. */
	//@{
	L3LocationAreaIdentity mLAI;
	L3CellIdentity mCI;
	L3FrequencyList mCellChannelDescription;
	L3FrequencyList mBCCHFrequencyList;
	L3NCCPermitted mNCCPermitted;
	L3ControlChannelDescription mControlChannelDescription;
	L3CellOptionsBCCH mCellOptionsBCCH;
	L3CellOptionsSACCH mCellOptionsSACCH;
	L3CellSelectionParameters mCellSelectionParameters;
	L3RACHControlParameters mRACHControlParameters;
	//@}
#ifndef NOL3
	/**@name Actual messages. */
	//@{
	L3SystemInformationType1 mSI1;
	L3SystemInformationType2 mSI2;
	L3SystemInformationType3 mSI3;
	L3SystemInformationType4 mSI4;
	L3SystemInformationType5 mSI5;
	L3SystemInformationType6 mSI6;
	//@}
#endif
	/**@name Encoded L2 frames to be sent on the BCCH. */
	//@{
	L2Frame mSI1Frame;
	L2Frame mSI2Frame;
	L2Frame mSI3Frame;
	L2Frame mSI4Frame;
	//@}
	/**@name Encoded L3 frames to be sent on the SACCH. */
	//@{
	L3Frame mSI5Frame;
	L3Frame mSI6Frame;
	//@}
	//@}

#ifndef NOL3
	/** The network "short name" displayed on some handsets. */
	L3NetworkName mShortName;
#endif

	public:
	
	

	/** Default parameters are for a single-ARFCN system. */
	GSMConfig(unsigned wNCC, unsigned wBCC,
			GSMBand,
			const L3LocationAreaIdentity&,
			const L3CellIdentity&,
			const char* wShortName="");
	
	/**@name Get references to L2 frames for BCCH SI messages. */
	//@{
	const L2Frame& SI1Frame() const { return mSI1Frame; }
	const L2Frame& SI2Frame() const { return mSI2Frame; }
	const L2Frame& SI3Frame() const { return mSI3Frame; }
	const L2Frame& SI4Frame() const { return mSI4Frame; }
	//@}
	/**@name Get references to L3 frames for SACCH SI messages. */
	//@{
	const L3Frame& SI5Frame() const { return mSI5Frame; }
	const L3Frame& SI6Frame() const { return mSI6Frame; }
	//@}

	/**@name Simple accessors */
	//@{
	GSMBand band() const { return mBand; }
	const Clock& clock() const { return mClock; }
	Clock& clock() { return mClock; }
	unsigned BCC() const { return mBCC; }
	Control::Pager& pager() { return mPager; }
	L3LocationAreaIdentity& LAI() { return mLAI; }
	L3CellIdentity& CI() { return mCI; }
#ifndef NOL3
	const L3NetworkName& shortName() const { return mShortName; }
#endif
	//@}

	/** Get the current master clock value. */
	Time time() const { return mClock.get(); }

	/** Return the BSIC, NCC:BCC. */
	unsigned BSIC() const { return (mNCC<<3) | mBCC; }

	/** RSSI target for closed loop power control. */
	int RSSITarget() const { return -10; }

#ifndef NOL3
	/**
		Re-encode the L2Frames for system information messages.
		Called whenever a beacon parameter is changed.
	*/
	void encodeSIFrames();
#endif

	protected:

	/** Find a minimum-load CCCH from a list. */
	CCCHLogicalChannel* minimumLoad(CCCHList &chanList);

	public:

	/**@name Manage CCCH subchannels. */
	//@{
	/** The add method is not mutex protected and should only be used during initialization. */
	void addAGCH(CCCHLogicalChannel* wCCCH) { mAGCHPool.push_back(wCCCH); }
	/** The add method is not mutex protected and should only be used during initialization. */
	void addPCH(CCCHLogicalChannel* wCCCH) { mPCHPool.push_back(wCCCH); }

	/** Return a minimum-load AGCH. */
	CCCHLogicalChannel* getAGCH() { return minimumLoad(mAGCHPool); }
	/** Return a minimum-load PCH. */
	CCCHLogicalChannel* getPCH() { return minimumLoad(mPCHPool); }
	//@}


	/**@name Manage SDCCH Pool. */
	//@{
	/** The add method is not mutex protected and should only be used during initialization. */
	void addSDCCH(SDCCHLogicalChannel *wSDCCH) { mSDCCHPool.push_back(wSDCCH); }
	/** Return a pointer to a usable channel. */
	SDCCHLogicalChannel *getSDCCH();
	//@}

	/**@name Manage TCH pool. */
	//@{
	/** The add method is not mutex protected and should only be used during initialization. */
	void addTCH(TCHFACCHLogicalChannel *wTCH) { mTCHPool.push_back(wTCH); }
	/** Return a pointer to a usable channel. */
	TCHFACCHLogicalChannel *getTCH();
	//@}

};



};	// GSM


/**@addtogroup Globals */
//@{
/** A single global GSMConfig object in the global namespace. */
extern GSM::GSMConfig gBTS;
//@}


#endif


// vim: ts=4 sw=4
