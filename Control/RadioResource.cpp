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

unsigned curT3122ms = 5000;
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
	curT3122ms += (random() % curT3122ms) / 2;
	unsigned max = gConfig.getNum("GSM.T3122Max");
	if (curT3122ms>max) curT3122ms=max;
	curT3122Lock.unlock();
	return retVal;
}

/** Restore T3122 to the base value.  */
void restoreT3122()
{
	curT3122Lock.lock();
	curT3122ms = gConfig.getNum("GSM.T3122Min");
	curT3122Lock.unlock();
}

//@}


/**
	Determine the channel type needed.
	This is based on GSM 04.08 9.1.8, Table 9.3 and 9.3a.
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
	// This code is formatted in the same order as GSM 04.08 Table 9.9.
	//
	// Emergency
	//
	// FIXME -- Use SDCCH to start emergency call, since some handsets don't support VEA.
	if ((RA>>5) == 0x05) return TCHFType;			// emergency call
	//
	// skip re-establishment cases
	//
	// "Answer to paging"
	//
	// We do not send the "any channel" paging indication, just SDCCH and TCH/F.
	// Cases where we sent SDCCH.
	if ((RA>>4) == 0x01) return SDCCHType;			// answer to paging, SDCCH
	// Cases where we sent TCH/F.
	if ((RA>>5) == 0x04) return TCHFType;			// answer to paging, any channel
	if ((RA>>4) == 0x02) return TCHFType;			// answer to paging, TCH/F
	// We don't send TCH/[FH], either.
	//
	// MOC or SDCCH procedures.
	//
	if ((RA>>5) == 0x07) return SDCCHType;			// MOC or SDCCH procedures
	//
	// Location updating.
	//
	if ((RA>>5) == 0x00) return SDCCHType;			// location updating
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


void Control::AccessGrantResponder(unsigned RA, const GSM::Time& when, float timingError)
{
	// RR Establishment.
	// Immediate Assignment procedure, "Answer from the Network"
	// GSM 04.08 3.3.1.1.3.
	// Given a request reference, try to allocate a channel
	// and send the assignment to the handset on the CCCH.
	// This GSM's version of medium access control.
	// Papa Legba, open that door...

	// FIXME -- Need to deal with initial timing advance, too.

	// Check "when" against current clock to see if we're too late.
	// Calculate maximum number of frames of delay.
	// See GSM 04.08 3.3.1.1.2 for the logic here.
	static const unsigned txInteger = gConfig.getNum("GSM.RACH.TxInteger");
	static const unsigned maxAge = GSM::RACHSpreadSlots[txInteger] + GSM::RACHWaitSParam[txInteger];
	// Check burst age.
	int age = gBTS.time() - when;
	LOG(INFO) << "RA=0x" << hex << RA << dec
		<< " when=" << when << " age=" << age << " TOA=" << timingError;
	if (age>maxAge) {
		LOG(NOTICE) << "ignoring RACH bust with age " << age;
		return;
	}

	// Screen for delay.
	if (gConfig.defines("GSM.MaxRACHDelay")) {
		if (timingError > gConfig.getNum("GSM.MaxRACHDelay")) return;
	}

	// Allocate the channel according to the needed type indicated by RA.
	// The returned channel is already open and ready for the transaction.
	LogicalChannel *LCH = NULL;
	switch (decodeChannelNeeded(RA)) {
		case TCHFType: LCH = gBTS.getTCH(); break;
		case SDCCHType: LCH = gBTS.getSDCCH(); break;
		// FIXME -- Should probably assign to an SDCCH and then send a reject of some kind.
		// If we don't support the service, assign to an SDCCH and we can reject it in L3.
		case UndefinedCHType:
			LOG(NOTICE) << "RACH burst for unsupported service";
			LCH = gBTS.getSDCCH();
			break;
		// We should never be here.
		default: assert(0);
	}

	// Get an AGCH to send on.
	CCCHLogicalChannel *AGCH = gBTS.getAGCH();
	// Someone had better have created a least one AGCH.
	assert(AGCH);

	// Nothing available?
	if (!LCH) {
		// Rejection, GSM 04.08 3.3.1.1.3.2.
		// BTW, emergency calls are not subject to T3122 hold-off.
		unsigned waitTime = curT3122()/1000;
		LOG(NOTICE) << "CONGESTION, T3122=" << waitTime;
		const L3ImmediateAssignmentReject reject(L3RequestReference(RA,when),waitTime);
		LOG(DEBUG) << "rejection, sending " << reject;
		AGCH->send(reject);
		return;
	}

	// Assignment, GSM 04.08 3.3.1.1.3.1.
	// Create the ImmediateAssignment message.
	int initialTA = (int)(timingError + 0.5F);
	if (initialTA<0) initialTA=0;
	if (initialTA>63) initialTA=63;
	const L3ImmediateAssignment assign(
		L3RequestReference(RA,when),
		LCH->channelDescription(),
		L3TimingAdvance(initialTA)
	);
	LOG(INFO) << "sending " << assign;
	AGCH->send(assign);

	// Reset exponential back-off upon successful allocation.
	restoreT3122();
}





void Control::PagingResponseHandler(const L3PagingResponse* resp, LogicalChannel* DCCH)
{
	assert(resp);
	assert(DCCH);
	LOG(INFO) << *resp;

	// If we got a TMSI, find the IMSI.
	L3MobileIdentity mobileID = resp->mobileIdentity();
	if (mobileID.type()==TMSIType) {
		const char *IMSI = gTMSITable.find(mobileID.TMSI());
		if (IMSI) mobileID = L3MobileIdentity(IMSI);
		else {
			// Don't try too hard to resolve.
			// The handset is supposed to respond with the same ID type as in the request.
			LOG(NOTICE) << "Paging Reponse with non-valid TMSI";
			// Cause 0x60 "Invalid mandatory information"
			DCCH->send(L3ChannelRelease(0x60));
			return;
		}
	}

	// Delete the Mobile ID from the paging list to free up CCCH bandwidth.
	// ... if it was not deleted by a timer already ...
	gBTS.pager().removeID(mobileID);

	// Find the transction table entry that was created when the phone was paged.
	// We have to look up by mobile ID since the paging entry may have been
	// erased before this handler was called.  That's too bad.
	// HACK -- We also flush stray transactions until we find what we 
	// are looking for.
	TransactionEntry transaction;
	while (true) {
		if (!gTransactionTable.find(mobileID,transaction)) {
			LOG(WARN) << "Paging Reponse with no transaction record for " << mobileID;
			// Cause 0x41 means "call already cleared".
			DCCH->send(L3ChannelRelease(0x41));
			return;
		}
		// We are looking for a mobile-terminated transaction.
		// The transaction controller will take it from here.
		switch (transaction.service().type()) {
			case L3CMServiceType::MobileTerminatedCall:
				MTCStarter(transaction, DCCH);
				return;
			case L3CMServiceType::TestCall:
				TestCall(transaction, DCCH);
				return;
			case L3CMServiceType::MobileTerminatedShortMessage:
				MTSMSController(transaction, DCCH);
				return;
			default:
				// Flush stray MOC entries.
				// There should not be any, but...
				LOG(WARN) << "flushing stray " << transaction.service().type() << " transaction entry";
				gTransactionTable.remove(transaction.ID());
				continue;
		}
	}
	// The transaction may or may not be cleared,
	// depending on the assignment type.
}



void Control::AssignmentCompleteHandler(const L3AssignmentComplete *confirm, TCHFACCHLogicalChannel *TCH)
{
	// The assignment complete handler is used to
	// tie together split transactions across a TCH assignment
	// in non-VEA call setup.

	assert(TCH);
	assert(confirm);
	LOG(DEBUG) << *confirm;

	// Check the transaction table to know what to do next.
	TransactionEntry transaction;
	if (!gTransactionTable.find(TCH->transactionID(),transaction)) {
		LOG(WARN) << "Assignment Complete with no transaction record";
		throw UnexpectedMessage();
	}
	LOG(INFO) << "service="<<transaction.service().type();

	// These "controller" functions don't return until the call is cleared.
	switch (transaction.service().type()) {
		case L3CMServiceType::MobileOriginatedCall:
			MOCController(transaction,TCH);
			break;
		case L3CMServiceType::MobileTerminatedCall:
			MTCController(transaction,TCH);
			break;
		default:
			LOG(WARN) << "unsupported service " << transaction.service();
			throw UnsupportedMessage(transaction.ID());
	}
	// If we got here, the call is cleared.
}








void Pager::addID(const L3MobileIdentity& newID, ChannelType chanType,
		unsigned wTransactionID, unsigned wLife)
{
	// Add a mobile ID to the paging list for a given lifetime.
	mLock.lock();
	// If this ID is already in the list, just reset its timer.
	// Uhg, another linear time search.
	// This would be faster if the paging list were ordered by ID.
	// But the list should usually be short, so it may not be worth the effort.
	bool renewed = false;
	for (PagingEntryList::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		if (lp->ID()==newID) {
			LOG(DEBUG) << newID << " already in table";
			lp->renew(wLife);
			mPageSignal.signal();
			mLock.unlock();
			return;
		}
	}
	// If this ID is new, put it in the list.
	mPageIDs.push_back(PagingEntry(newID,chanType,wTransactionID,wLife));
	LOG(INFO) << newID << " added to table";
	mPageSignal.signal();
	mLock.unlock();
}


unsigned Pager::removeID(const L3MobileIdentity& delID)
{
	// Return the associated transaction ID, or 0 if none found.
	unsigned retVal = 0;
	LOG(INFO) << delID;
	mLock.lock();
	for (PagingEntryList::iterator lp = mPageIDs.begin(); lp != mPageIDs.end(); ++lp) {
		if (lp->ID()==delID) {
			retVal = lp->transactionID();
			mPageIDs.erase(lp);
			break;
		}
	}
	mLock.unlock();
	return retVal;
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
			// DO NOT remove the transaction entry here.
			// It may be in use in an active call.
			LOG(INFO) << "erasing " << lp->ID();
			lp=mPageIDs.erase(lp);
		}
	}

	LOG(INFO) << "paging " << mPageIDs.size() << " mobile(s)";

	// Page remaining entries, two at a time if possible.
	// These PCH send operations are non-blocking.
	lp = mPageIDs.begin();
	while (lp != mPageIDs.end()) {
		// FIXME -- This completely ignores the paging groups.
		// HACK -- So we send every page twice.
		// That will probably mean a different Pager for each subchannel.
		// See GSM 04.08 10.5.2.11 and GSM 05.02 6.5.2.
		CCCHLogicalChannel *PCH = gBTS.getPCH();
		assert(PCH);
		const L3MobileIdentity& id1 = lp->ID();
		ChannelType type1 = lp->type();
		++lp;
		if (lp==mPageIDs.end()) {
			// Just one ID left?
			LOG(DEBUG) << "paging " << id1;
			PCH->send(L3PagingRequestType1(id1,type1));
			PCH->send(L3PagingRequestType1(id1,type1));
			break;
		}
		// Page by pairs when possible.
		const L3MobileIdentity& id2 = lp->ID();
		ChannelType type2 = lp->type();
		++lp;
		LOG(DEBUG) << "paging " << id1 << " and " << id2;
		PCH->send(L3PagingRequestType1(id1,type1,id2,type2));
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
		LOG(DEBUG) << "Pager waiting for " << load << " multiframes";
		if (load) sleepFrames(51*load);

		// Page the list.
		// If there is nothing to page,
		// wait for a new entry in the list.
		if (!pageAll()) {
			LOG(DEBUG) << "Pager blocking for signal";
			mLock.lock();
			while (mPageIDs.size()==0) mPageSignal.wait(mLock);
			mLock.unlock();
		}
	}
}




// vim: ts=4 sw=4
