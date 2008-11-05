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
	(c) Kestrel Signal Processing, Inc. 2007, 2008
	David Burgess, Raffi Sevlian
*/

#include "GSMConfig.h"
#include "GSMTransfer.h"
#include "GSMLogicalChannel.h"


/*
	Compilation flags
	NOL3	Minimal references to L3
*/

using namespace std;
using namespace GSM;



#ifndef NOL3
GSMConfig::GSMConfig(unsigned wNCC, unsigned wBCC,
		GSMBand wBand,
		const L3LocationAreaIdentity& wLAI,
		const L3CellIdentity& wCI,
		const char* wShortName)
	:mNCC(wNCC),mBCC(wBCC),
	mBand(wBand),
	mLAI(wLAI),mCI(wCI),
	mSI5Frame(UNIT_DATA),mSI6Frame(UNIT_DATA),
	mShortName(wShortName)
{
	assert(wNCC<8);
	assert(wBCC<8);
	encodeSIFrames();
}
#else
GSMConfig::GSMConfig(unsigned wNCC, unsigned wBCC,
		GSMBand wBand,
		const L3LocationAreaIdentity& wLAI,
		const L3CellIdentity& wCI,
		const char* wShortName)
	:mNCC(wNCC),mBCC(wBCC),
	mBand(wBand),
	mLAI(wLAI),mCI(wCI)
{
	assert(wNCC<8);
	assert(wBCC<8);
}
#endif


#ifndef NOL3
void GSMConfig::encodeSIFrames()
{
	// an L3 frame to use
	L3Frame l3(UNIT_DATA);

	// SI1
	mSI1.RACHControlParameters(mRACHControlParameters);
	mSI1.cellChannelDescription(mCellChannelDescription);
	mSI1.write(l3);
	L2Header SI1Header(L2Length(l3.length()));
	mSI1Frame = L2Frame(SI1Header,l3);
	DCOUT("mSI1Frame " << mSI1Frame);

	// SI2
	mSI2.BCCHFrequencyList(mBCCHFrequencyList);
	mSI2.NCCPermitted(mNCCPermitted);
	mSI2.RACHControlParameters(mRACHControlParameters);
	mSI2.write(l3);
	L2Header SI2Header(L2Length(l3.length()));
	mSI2Frame = L2Frame(SI2Header,l3);
	DCOUT("mSI2Frame " << mSI2Frame);

	// SI3
	mSI3.CI(mCI);
	mSI3.LAI(mLAI);
	mSI3.controlChannelDescription(mControlChannelDescription);
	mSI3.cellOptions(mCellOptionsBCCH);
	mSI3.cellSelectionParameters(mCellSelectionParameters);
	mSI3.RACHControlParameters(mRACHControlParameters);
	mSI3.write(l3);
	L2Header SI3Header(L2Length(l3.length()));
	mSI3Frame = L2Frame(SI3Header,l3);
	DCOUT("mSI3Frame " << mSI3Frame);

	// SI4
	mSI4.LAI(mLAI);
	mSI4.cellSelectionParameters(mCellSelectionParameters);
	mSI4.RACHControlParameters(mRACHControlParameters);
	mSI4.write(l3);
	L2Header SI4Header(L2Length(l3.length()));
	mSI4Frame = L2Frame(SI4Header,l3);
	DCOUT("mSI4Frame " << mSI4Frame);

	// SI5
	mSI5.BCCHFrequencyList(mBCCHFrequencyList);
	mSI5.write(mSI5Frame);
	DCOUT("mSI5Frame " << mSI5Frame);

	// SI6
	mSI6.CI(mCI);
	mSI6.LAI(mLAI);
	mSI6.cellOptions(mCellOptionsSACCH);
	mSI6.NCCPermitted(mNCCPermitted);
	mSI6.write(mSI6Frame);
	DCOUT("mSI6Frame " << mSI6Frame);

}
#endif





CCCHLogicalChannel* GSMConfig::minimumLoad(CCCHList &chanList)
{
	if (chanList.size()==0) return NULL;
	CCCHList::iterator chan = chanList.begin();
	CCCHLogicalChannel *retVal = *chan;
	unsigned minLoad = (*chan)->load();
	++chan;
	while (chan!=chanList.end()) {
		unsigned thisLoad = (*chan)->load();
		if ((thisLoad<minLoad) ||
			((thisLoad==minLoad) && (random()%2)) )
		{
			minLoad = thisLoad;
			retVal = *chan;
		}
		++chan;
	}
	return retVal;
}






template <class ChanType> ChanType* getChan(vector<ChanType*> chanList)
{
	const unsigned sz = chanList.size();
	if (sz==0) return NULL;
	// Start the search from a random point in the list.
	unsigned pos = random() % sz;
	for (unsigned i=0; i<sz; i++) {
		ChanType *chan = chanList[pos];
		if (chan->recyclable()) {
			chan->open();
			return chan;
		}
		pos = (pos+1) % sz;
	}
	return NULL;
}





SDCCHLogicalChannel *GSMConfig::getSDCCH()
{
	mLock.lock();
	SDCCHLogicalChannel *chan = getChan<SDCCHLogicalChannel>(mSDCCHPool);
	mLock.unlock();
	return chan;
}


TCHFACCHLogicalChannel *GSMConfig::getTCH()
{
	mLock.lock();
	TCHFACCHLogicalChannel *chan = getChan<TCHFACCHLogicalChannel>(mTCHPool);
	mLock.unlock();
	return chan;
}


// vim: ts=4 sw=4
