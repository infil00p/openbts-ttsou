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



#ifndef GSMCONFIG_H
#define GSMCONFIG_H

#include <vector>

#include "ControlCommon.h"

#include "GSML3RRElements.h"
#include "GSML3CommonElements.h"
#include "GSML3RRMessages.h"



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

	time_t mStartTime;

	L3LocationAreaIdentity mLAI;
	char mShortName[94]; // GSM 03.38 6.1.2.2.1

	public:
	
	

	/** All parameters come from gConfig. */
	GSMConfig();
	
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

	/** Get the current master clock value. */
	Time time() const { return mClock.get(); }

	/**@name Accessors. */
	//@{
	Control::Pager& pager() { return mPager; }
	GSMBand band() const { return mBand; }
	unsigned BCC() const { return mBCC; }
	unsigned NCC() const { return mNCC; }
	GSM::Clock& clock() { return mClock; }
	const L3LocationAreaIdentity& LAI() const { return mLAI; }
	const char *shortName() const { return mShortName; }
	//@}

	/** Return the BSIC, NCC:BCC. */
	unsigned BSIC() const { return (mNCC<<3) | mBCC; }

	/** RSSI target for closed loop power control. */
	int RSSITarget() const { return -10; }

	/**
		Re-encode the L2Frames for system information messages.
		Called whenever a beacon parameter is changed.
	*/
	void regenerateBeacon();

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
	/** Return true if an SDCCH is available, but do not allocate it. */
	bool SDCCHAvailable() const;
	/** Return number of total SDCCH. */
	unsigned SDCCHTotal() const { return mSDCCHPool.size(); }
	/** Return number of active SDCCH. */
	unsigned SDCCHActive() const;
	/** Just a reference to the SDCCH pool. */
	const SDCCHList& SDCCHPool() const { return mSDCCHPool; }
	//@}

	/**@name Manage TCH pool. */
	//@{
	/** The add method is not mutex protected and should only be used during initialization. */
	void addTCH(TCHFACCHLogicalChannel *wTCH) { mTCHPool.push_back(wTCH); }
	/** Return a pointer to a usable channel. */
	TCHFACCHLogicalChannel *getTCH();
	/** Return true if an TCH is available, but do not allocate it. */
	bool TCHAvailable() const;
	/** Return number of total TCH. */
	unsigned TCHTotal() const { return mTCHPool.size(); }
	/** Return number of active TCH. */
	unsigned TCHActive() const;
	/** Just a reference to the TCH pool. */
	const TCHList& TCHPool() const { return mTCHPool; }
	//@}


	/** Return number of seconds since starting. */
	time_t uptime() const { return ::time(NULL)-mStartTime; }
};



};	// GSM


/**@addtogroup Globals */
//@{
/** A single global GSMConfig object in the global namespace. */
extern GSM::GSMConfig gBTS;
//@}


#endif


// vim: ts=4 sw=4
