/**@file GSM Radio Resource procedures, GSM 04.18 and GSM 04.08. */

/*
* Copyright 2008 Free Software Foundation, Inc.
*

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

* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*/

/*
Contributors:
David A. Burgess, dburgess@ketsrelsp.com
Raffi Sevlian, raffisev@gmail.com
Harvind S. Samra, hssamra@kestrelsp.com
*/



#include <stdio.h>
#include <stdlib.h>
#include <list>

#include "ControlCommon.h"
#include "GSMLogicalChannel.h"
#include "GSMConfig.h"





using namespace std;
using namespace GSM;
using namespace Control;




/**@name Mechanisms for exponential backoff of T3122, the access holdoff timer. */
//@{

unsigned curT3122ms = T3122ms;
Mutex curT3122Lock;

/**
	Get latest value of T3122 and apply exponential backoff.
	@return Current T3122 value in ms.
*/
unsigned curT3122()
{
	curT3122Lock.lock();
	unsigned retVal = curT3122ms;
	// Apply exponential back-off.
	curT3122ms += random() % curT3122ms;
	if (curT3122ms>255000) curT3122ms=255000;
	curT3122Lock.unlock();
	return retVal;
}

/** Restore T3122 to the base value.  */
void restoreT3122()
{
	curT3122Lock.lock();
	curT3122ms = T3122ms;
	curT3122Lock.unlock();
}

//@}


void Control::AccessGrantResponder(unsigned RA, const GSM::Time& when)
{
	// RR Establishment.
	// Immediate Assignment procedure, "Answer from the Network"
	// GSM 04.08 3.3.1.1.3.
	// Given a request reference, try to allocate a channel
	// and send the assignment to the handset on the CCCH.

	CLDCOUT("AccessGrantResponder RA=" << RA << " when=" << when);

	// FIXME -- Check "when" against current clock to see if we're too late.

	// Check the request type to see if it's a service we don't even support.
	// If we don't support it, ignore it.
	if ((RA&0xe0)==0x70) return;	// GPRS
	if ((RA&0xe0)==0x60) return;	// TCH/H reestablishment & some reserved codes
	if (RA==0x67) return;			// LMU
	if (RA==0xef) return;			// reserved

	// Get an AGCH to send on.
	CCCHLogicalChannel *AGCH = gBTS.getAGCH();
	assert(AGCH);

	// Get an SDCCH to assign to.
	SDCCHLogicalChannel *SDCCH = gBTS.getSDCCH();

	// Nothing available?
	if (!SDCCH) {
		// Rejection, GSM 04.08 3.3.1.1.3.2.
		// Emergency calls are not subject to T3122 hold-off.
		// They are not handled as a special case because the
		// MS will ignore the T3122 setting.
		CERR("NOTICE -- SDCCH CONGESTION");
		unsigned waitTime = curT3122()/1000;
		CLDCOUT("AccessGrantResponder: assginment reject, wait time " << waitTime);
		const L3ImmediateAssignmentReject reject(L3RequestReference(RA,when),waitTime);
		CLDCOUT("AccessGrantResponder sending " << reject);
		AGCH->send(reject);
		return;
	}

	// Assignment, GSM 04.08 3.3.1.1.3.1.
	// Create the ImmediateAssignment message.
	// For most of the message, default IE values are correct.
	const L3ImmediateAssignment assign(L3RequestReference(RA,when),SDCCH->channelDescription());
	CLDCOUT("AccessGrantResponder sending " << assign);
	AGCH->send(assign);
	SDCCH->open();

	// Reset exponential back-off upon successful allocation.
	restoreT3122();
}





void Control::PagingResponseHandler(const L3PagingResponse* resp, SDCCHLogicalChannel* SDCCH)
{
	CLDCOUT("PagingResponseHandler " << *resp);

	// FIXME -- Delete the Mobile ID from the paging list to free up CCCH bandwidth.

	// FIXME -- Check to see reason for paging response, and
	// if not a legitimate reason, need to release the channel.

	// FIXME -- Check the transaction table to see if the call is still valid.
#ifndef PAGERTEST
	// For now, assume MTC.
	MTCStarter(resp, SDCCH);
#else
	COUT("starting MTC...");
#endif
}








void Pager::addID(const L3MobileIdentity& newID, unsigned wLife)
{
	// Add a mobile ID to the paging list for a given lifetime.

	CLDCOUT("Pager::addID " << newID);
	mLock.lock();
	// If this ID is already in the list, just reset its timer.
	// TODO -- It would be nice if this were not a linear time operation.
	bool renewed = false;
	for (list<PagingEntry>::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		CLDCOUT("Pager::addID " << newID << " already in table");
		if (lp->ID()==newID) {
			lp->renew(wLife);
			renewed = true;
			break;
		}
	}
	// If this ID is new, put it in the list.
	if (!renewed) {
		mPageIDs.push_back(PagingEntry(newID,wLife));
		CLDCOUT("Pager::addID " << newID << " added to table");
	}
	// Signal in case the paging loop is waiting for new entries.
	mPageSignal.signal();
	mLock.unlock();
}



unsigned Pager::pageAll()
{
	// Traverse the full list and page all IDs.
	// Return the number of IDs paged.

	unsigned numPaged = 0;

	mLock.lock();

	// Clear expired entries.
	for (list<PagingEntry>::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		if (!lp->expired()) continue;
		CLDCOUT("Pager::pageAll erasing " << lp->ID());
		mPageIDs.erase(lp);
		// FIXME -- We should delete the Transaction Table entry here.
	}

	// Page remaining entries, two at a time if possible.
	list<PagingEntry>::iterator lp = mPageIDs.begin();
	while (lp != mPageIDs.end()) {
		// HACK -- Just pick the minimum load channel.
		// FIXME -- This completely ignores the paging goups, GSM 04.08 10.5.2.11 and GSM 05.02 6.5.2.
		GSM::CCCHLogicalChannel *PCH = gBTS.getPCH();
		assert(PCH);
		const L3MobileIdentity& id1 = lp->ID(); ++lp;
		if (lp==mPageIDs.end()) {
			// Just one ID left?
			//CLDCOUT("Pager::pageAll paging " << id1);
			PCH->send(L3PagingRequestType1(id1));
			numPaged++;
			break;
		}
		// Page by pairs when possible.
		const L3MobileIdentity& id2 = lp->ID(); ++lp;
		//CLDCOUT("Pager::pageAll paging " << id1 << " and " << id2);
		PCH->send(L3PagingRequestType1(id1,id2));
		numPaged += 2;
	}
	
	mLock.unlock();

	return numPaged;
}



void Pager::start()
{
	if (mRunning) return;
	mRunning=true;
	mPagingThread.start((void* (*)(void*))PagerServiceLoopAdapter, (void*)this);
}



void* Control::PagerServiceLoopAdapter(Pager *pager)
{
	pager->serviceLoop();
	return NULL;
}

void Pager::serviceLoop()
{
	while (mRunning) {
		// Wait for pending activity to clear the channel.
		// This wait is what causes PCH to have lower priority than AGCH.
		unsigned load = gBTS.getPCH()->load();
		while (load>0) {
			DCOUT("Pager blocking for " << load << " multiframes");
			sleepFrames(51*load);
			load = gBTS.getPCH()->load();
		}

		// If there was nothing to page,
		// wait for a new entry in the list.
		if (!pageAll()) {
			DCOUT("Pager blocking for signal");
			mPageSignal.wait(mLock);
		}
	}
}




// vim: ts=4 sw=4
