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
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.
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

	// Allocate the channel according to the needed type indicated by RA.
	// The returned channel is already open and ready for the transaction.
	LogicalChannel *LCH = NULL;
	switch (decodeChannelNeeded(RA)) {
		case TCHFType: LCH = gBTS.getTCH(); break;
		case SDCCHType: LCH = gBTS.getSDCCH(); break;
		// If we don't support it, ignore it.
		default: return;
	}

	// Get an AGCH to send on.
	CCCHLogicalChannel *AGCH = gBTS.getAGCH();
	assert(AGCH);

	// Nothing available?
	if (!LCH) {
		// Rejection, GSM 04.08 3.3.1.1.3.2.
		// BTW, emergency calls are not subject to T3122 hold-off.
		CERR("NOTICE -- Access Grant CONGESTION at uptime " << gBTS.uptime() << " frame " << gBTS.time());
		//abort(); // HACK DEBUG HACK
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

	// Reset exponential back-off upon successful allocation.
	restoreT3122();
}





void Control::PagingResponseHandler(const L3PagingResponse* resp, LogicalChannel* DCCH)
{
	assert(resp);
	assert(DCCH);
	CLDCOUT("PagingResponseHandler " << *resp);

	// Map a TMSI to an IMSI if needed.
	L3MobileIdentity mobileID = resp->mobileIdentity();
	if (mobileID.type()==TMSIType) {
		const char *IMSI = gTMSITable.find(mobileID.TMSI());
		if (IMSI) mobileID = L3MobileIdentity(IMSI);
	}

	// Delete the Mobile ID from the paging list to free up CCCH bandwidth.
	gBTS.pager().removeID(mobileID);

	// Find the transction table entry that was created when the phone was paged.
	CLDCOUT("PagingResponseHandler find TransactionEntry for " << mobileID);
	TransactionEntry transaction;
	if (!gTransactionTable.find(mobileID,transaction)) {
		CLDCOUT("NOTICE -- Paging Reponse with no transaction record");
		throw UnexpectedMessage();
	}
	CLDCOUT("PagingResponseHandler service="<<transaction.service().type());

	// These "controller" functions don't return until the call is cleared.
	switch (transaction.service().type()) {
		case L3CMServiceType::MobileTerminatedCall:
			MTCStarter(transaction, DCCH);
			break;
		default:
			CLDCOUT("NOTICE -- request for unsupported service " << transaction.service());
			// Remove the bogus transaction.
			gTransactionTable.remove(transaction.ID());
			throw UnsupportedMessage();
	}

	// If we got here, the call is cleared.
	clearTransactionHistory(transaction);
}



void Control::AssignmentCompleteHandler(const L3AssignmentComplete *confirm, TCHFACCHLogicalChannel *TCH)
{
	assert(TCH);
	assert(confirm);
	CLDCOUT("AssignmentCompleteHandler " << *confirm);

	// Check the transaction table to know what to do next.
	TransactionEntry transaction;
	if (!gTransactionTable.find(TCH->transactionID(),transaction)) {
		CLDCOUT("NOTICE -- Assignment Complete with no transaction record");
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
	clearTransactionHistory(transaction);
}








void Pager::addID(const L3MobileIdentity& newID, ChannelType chanType, unsigned wLife)
{
	// Add a mobile ID to the paging list for a given lifetime.

	CLDCOUT("Pager::addID " << newID);
	mLock.lock();
	// If this ID is already in the list, just reset its timer.
	bool renewed = false;
	for (PagingEntryList::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		if (lp->ID()==newID) {
			CLDCOUT("Pager::addID " << newID << " already in table");
			lp->renew(wLife);
			mPageSignal.signal();
			mLock.unlock();
			return;
		}
	}
	// If this ID is new, put it in the list.
	mPageIDs.push_back(PagingEntry(newID,chanType,wLife));
	CLDCOUT("Pager::addID " << newID << " added to table");
	mPageSignal.signal();
	mLock.unlock();
}


void Pager::removeID(const L3MobileIdentity& delID)
{
	CLDCOUT("Pager::removeID " << delID);
	mLock.lock();
	for (PagingEntryList::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		if (lp->ID()==delID) {
			mPageIDs.erase(lp);
			break;
		}
	}
	mLock.unlock();
}



unsigned Pager::pageAll()
{
	// Traverse the full list and page all IDs.
	// Remove expired IDs.
	// Return the number of IDs paged.
	// This is a linear time operation.

	mLock.lock();

	// Clear expired entries.
	PagingEntryList::iterator lp = mPageIDs.begin();
	while (lp != mPageIDs.end()) {
		if (!lp->expired()) ++lp;
		else {
			// FIXME -- We should delete the corresponding Transaction Table entry here, too.
			CLDCOUT("Pager::pageAll erasing " << lp->ID());
			lp=mPageIDs.erase(lp);
		}
	}

	CLDCOUT("Pager::pageAll paging " << mPageIDs.size() << " mobile(s)");

	// Page remaining entries, two at a time if possible.
	// These PCH send operations are non-blocking.
	lp = mPageIDs.begin();
	while (lp != mPageIDs.end()) {
		// FIXME -- This completely ignores the paging groups.
		// That will probably mean a different Pager for each subchannel.
		// See GSM 04.08 10.5.2.11 and GSM 05.02 6.5.2.
		CCCHLogicalChannel *PCH = gBTS.getPCH();
		assert(PCH);
		const L3MobileIdentity& id1 = lp->ID();
		ChannelType type1 = lp->type();
		++lp;
		if (lp==mPageIDs.end()) {
			// Just one ID left?
			//CLDCOUT("Pager::pageAll paging " << id1);
			PCH->send(L3PagingRequestType1(id1,type1));
			break;
		}
		// Page by pairs when possible.
		const L3MobileIdentity& id2 = lp->ID();
		ChannelType type2 = lp->type();
		++lp;
		//CLDCOUT("Pager::pageAll paging " << id1 << " and " << id2);
		PCH->send(L3PagingRequestType1(id1,type1,id2,type2));
	}
	
	mLock.unlock();

	return mPageIDs.size();
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
		DCOUT("Pager waiting for " << load << " multiframes");
		sleepFrames(51*load);

		// Page the list.
		// If there is nothing to page,
		// wait for a new entry in the list.
		if (!pageAll()) {
			DCOUT("Pager blocking for signal");
			mLock.lock();
			while (mPageIDs.size()==0) mPageSignal.wait(mLock);
			mLock.unlock();
		}
	}
}




// vim: ts=4 sw=4
