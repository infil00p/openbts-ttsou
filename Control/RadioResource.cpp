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


/**
	Determine the channel type needed.
	This is based on GSM 04.08 Table 9.3 and 9.3a.
	The following is assumed about the global BTS capabilities:
	- We do not support "new establishment causes" and NECI is 0.
	- We do not support call reestablishment.
	- We do not support GPRS.
	@param RA The request reference from the channel request message.
	@return channel type code, undefined if not a supported service
*/
ChannelType decodeChannelNeeded(unsigned RA)
{
	// These values assume NECI is 0.
	// This code is formatted so that it lines up easily with GSM 04.08 Table 9.9.
	//
	if ((RA>>5) == 0x05) return TCHFType;			// emergency call
	//
	// skip re-establishment cases
	//
	// "Answer to paging"
	if ((RA>>5) == 0x04) return SDCCHType;			// answer to paging, any channel
	if ((RA>>4) == 0x01) return SDCCHType;			// answer to paging, SDCCH
	if ((RA>>4) == 0x02) return TCHFType;			// answer to paging, TCH/F
	if ((RA>>4) == 0x03) return TCHFType;			// answer to paging, TCH/F or TCH/H
	//
	if ((RA>>5) == 0x07) return SDCCHType;			// MOC or SDCCH procedures
	//
	if ((RA>>4) == 0x04) return TCHFType;			// MOC
	//
	// skip originating data call cases
	//
	if ((RA>>5) == 0x00) return SDCCHType;			// location updating
	//
	if ((RA>>4) == 0x00) return SDCCHType;			// location updating
	//
	// skip packet (GPRS) cases
	//
	// skip LMU case
	//
	// skip reserved cases
	//
	// Anything else falls through to here.
	return UndefinedCHType;
}


void Control::AccessGrantResponder(unsigned RA, const GSM::Time& when)
{
	// RR Establishment.
	// Immediate Assignment procedure, "Answer from the Network"
	// GSM 04.08 3.3.1.1.3.
	// Given a request reference, try to allocate a channel
	// and send the assignment to the handset on the CCCH.
	// This GSM's version of medium access control.
	// Papa Legba, open that door...

	CLDCOUT("AccessGrantResponder RA=" << RA << " when=" << when);

	// FIXME -- Check "when" against current clock to see if we're too late.

	// Determine the channel type needed.
	ChannelType chanNeeded = decodeChannelNeeded(RA);

	// Allocate the channel.
	LogicalChannel *LCH = NULL;
	if (chanNeeded==TCHFType) LCH = gBTS.getTCH();
	if (chanNeeded==SDCCHType) LCH = gBTS.getSDCCH();
	// If we don't support it, ignore it.
	if (LCH==NULL) return;

	// Get an AGCH to send on.
	CCCHLogicalChannel *AGCH = gBTS.getAGCH();
	assert(AGCH);

	// Nothing available?
	if (!LCH) {
		// Rejection, GSM 04.08 3.3.1.1.3.2.
		// BTW, emergency calls are not subject to T3122 hold-off.
		CERR("NOTICE -- Access Grant CONGESTION");
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
	const L3ImmediateAssignment assign(L3RequestReference(RA,when),LCH->channelDescription());
	CLDCOUT("AccessGrantResponder sending " << assign);
	AGCH->send(assign);
	// This was opened by the gBTS.getXXX() method.  LCH->open();

	// Reset exponential back-off upon successful allocation.
	restoreT3122();
}





void Control::PagingResponseHandler(const L3PagingResponse* resp, LogicalChannel* DCCH)
{
	assert(resp);
	assert(DCCH);
	CLDCOUT("PagingResponseHandler " << *resp);

	// FIXME -- Delete the Mobile ID from the paging list to free up CCCH bandwidth.

	// FIXME -- Check to see reason for paging response, and
	// if not a legitimate reason, need to release the channel.

	// FIXME -- Check the transaction table to see if the call is still valid.

#ifndef PAGERTEST
	// For now, assume MTC.
	MTCStarter(resp, DCCH);
#else
	COUT("starting MTC...");
#endif
}



void Control::AssignmentCompleteHandler(const L3AssignmentComplete *confirm, TCHFACCHLogicalChannel *TCH)
{
	assert(TCH);
	assert(confirm);

	// Check the transaction table to know what to do next.
	TransactionEntry transaction;
	if (!gTransactionTable.find(TCH->transactionID(),transaction)) {
		CLDCOUT("NOTICE -- Assignment Complete from channel with no transaction");
		throw UnexpectedMessage();
	}
	CLDCOUT("AssignmentCompleteHandler service="<<transaction.service().type());
	// These "controller" functions don't return until the call is cleared.
	switch (transaction.service().type()) {
		case L3CMServiceType::MobileOriginatedCall:
			MOCController(transaction,TCH);
			break;
		case L3CMServiceType::MobileTerminatedCall:
			MTCController(transaction,TCH);
			break;
		default:
			CLDCOUT("NOTICE -- request for unsupported service " << transaction.service());
			throw UnsupportedMessage();
	}
	// If we got here, the call is cleared.
	clearTransactionHistory(TCH->transactionID());
}








void Pager::addID(const L3MobileIdentity& newID, ChannelType chanType, unsigned wLife)
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
		mPageIDs.push_back(PagingEntry(newID,chanType,wLife));
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
		// FIXME -- This completely ignores the paging groups, GSM 04.08 10.5.2.11 and GSM 05.02 6.5.2.
		CCCHLogicalChannel *PCH = gBTS.getPCH();
		assert(PCH);
		const L3MobileIdentity& id1 = lp->ID();
		ChannelType type1 = lp->type();
		++lp;
		if (lp==mPageIDs.end()) {
			// Just one ID left?
			//CLDCOUT("Pager::pageAll paging " << id1);
			PCH->send(L3PagingRequestType1(id1,type1));
			numPaged++;
			break;
		}
		// Page by pairs when possible.
		const L3MobileIdentity& id2 = lp->ID();
		ChannelType type2 = lp->type();
		++lp;
		//CLDCOUT("Pager::pageAll paging " << id1 << " and " << id2);
		PCH->send(L3PagingRequestType1(id1,type1,id2,type2));
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
