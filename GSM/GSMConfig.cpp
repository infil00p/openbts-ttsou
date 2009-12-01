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


#include "GSMConfig.h"
#include "GSMTransfer.h"
#include "GSMLogicalChannel.h"
#include <Logger.h>



using namespace std;
using namespace GSM;



GSMConfig::GSMConfig()
	:mSI5Frame(UNIT_DATA),mSI6Frame(UNIT_DATA),
	mStartTime(::time(NULL))
{
	mBand = (GSMBand)gConfig.getNum("GSM.Band");
	regenerateBeacon();
}




void GSMConfig::regenerateBeacon()
{
	// Update everything from the configuration.

	// BSIC components
	mNCC = gConfig.getNum("GSM.NCC");
	assert(mNCC<8);
	mBCC = gConfig.getNum("GSM.BCC");
	assert(mBCC<8);

	// MCC/MNC/LAC
	mLAI = L3LocationAreaIdentity();

	// Short name
	const char* shortName = gConfig.getStr("GSM.ShortName");
	assert(strlen(shortName)<=93);
	strcpy(mShortName,shortName);
	
	// Now regenerate all of the system information messages.

	// an L3 frame to use
	L3Frame l3(UNIT_DATA);

	// SI1
	L3SystemInformationType1 SI1;
	SI1.write(l3);
	L2Header SI1Header(L2Length(l3.length()));
	mSI1Frame = L2Frame(SI1Header,l3);
	LOG(DEBUG) << "mSI1Frame " << mSI1Frame;

	// SI2
	L3SystemInformationType2 SI2;
	SI2.write(l3);
	L2Header SI2Header(L2Length(l3.length()));
	mSI2Frame = L2Frame(SI2Header,l3);
	LOG(DEBUG) << "mSI2Frame " << mSI2Frame;

	// SI3
	L3SystemInformationType3 SI3;
	SI3.write(l3);
	L2Header SI3Header(L2Length(l3.length()));
	mSI3Frame = L2Frame(SI3Header,l3);
	LOG(DEBUG) << "mSI3Frame " << mSI3Frame;

	// SI4
	L3SystemInformationType4 SI4;
	SI4.write(l3);
	L2Header SI4Header(L2Length(l3.length()));
	mSI4Frame = L2Frame(SI4Header,l3);
	LOG(DEBUG) << "mSI4Frame " << mSI4Frame;

	// SI5
	L3SystemInformationType5 SI5;
	SI5.write(mSI5Frame);
	LOG(DEBUG) << "mSI5Frame " << mSI5Frame;

	// SI6
	L3SystemInformationType6 SI6;
	SI6.write(mSI6Frame);
	LOG(DEBUG) "mSI6Frame " << mSI6Frame;

}





CCCHLogicalChannel* GSMConfig::minimumLoad(CCCHList &chanList)
{
	if (chanList.size()==0) return NULL;
	CCCHList::iterator chan = chanList.begin();
	CCCHLogicalChannel *retVal = *chan;
	unsigned minLoad = (*chan)->load();
	++chan;
	while (chan!=chanList.end()) {
		unsigned thisLoad = (*chan)->load();
		if (thisLoad<minLoad) {
			minLoad = thisLoad;
			retVal = *chan;
		}
		++chan;
	}
	return retVal;
}






template <class ChanType> ChanType* getChan(vector<ChanType*>& chanList)
{
	const unsigned sz = chanList.size();
	if (sz==0) return NULL;
	// Start the search from a random point in the list.
	//unsigned pos = random() % sz;
	// HACK -- Try in-order allocation for debugging.
	for (unsigned i=0; i<sz; i++) {
		ChanType *chan = chanList[i];
		//ChanType *chan = chanList[pos];
		if (chan->recyclable()) return chan;
		//pos = (pos+1) % sz;
	}
	return NULL;
}





SDCCHLogicalChannel *GSMConfig::getSDCCH()
{
	mLock.lock();
	SDCCHLogicalChannel *chan = getChan<SDCCHLogicalChannel>(mSDCCHPool);
	if (chan) chan->open();
	mLock.unlock();
	return chan;
}


TCHFACCHLogicalChannel *GSMConfig::getTCH()
{
	mLock.lock();
	TCHFACCHLogicalChannel *chan = getChan<TCHFACCHLogicalChannel>(mTCHPool);
	if (chan) chan->open();
	mLock.unlock();
	return chan;
}



template <class ChanType> bool chanAvailable(const vector<ChanType*>& chanList)
{
	for (unsigned i=0; i<chanList.size(); i++) {
		ChanType *chan = chanList[i];
		if (chanList[i]->recyclable()) return true;
	}
	return false;
}



bool GSMConfig::SDCCHAvailable() const
{
	mLock.lock();
	bool retVal = chanAvailable<SDCCHLogicalChannel>(mSDCCHPool);
	mLock.unlock();
	return retVal;
}

bool GSMConfig::TCHAvailable() const
{
	mLock.lock();
	bool retVal = chanAvailable<TCHFACCHLogicalChannel>(mTCHPool);
	mLock.unlock();
	return retVal;
}







template <class ChanType> unsigned countActive(const vector<ChanType*>& chanList)
{
	unsigned active = 0;
	const unsigned sz = chanList.size();
	// Start the search from a random point in the list.
	for (unsigned i=0; i<sz; i++) {
		if (!chanList[i]->recyclable()) active++;
	}
	return active;
}


unsigned GSMConfig::SDCCHActive() const
{
	return countActive(mSDCCHPool);
}

unsigned GSMConfig::TCHActive() const
{
	return countActive(mTCHPool);
}


// vim: ts=4 sw=4
