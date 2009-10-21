/**@file Common-use functions for the control layer. */

/*
* Copyright 2008 Free Software Foundation, Inc.
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


#include "ControlCommon.h"

#include <GSMLogicalChannel.h>
#include <GSML3Message.h>
#include <GSML3CCMessages.h>
#include <GSML3RRMessages.h>
#include <GSML3MMMessages.h>
#include <GSMConfig.h>

#include <SIPEngine.h>
#include <SIPInterface.h>


using namespace std;
using namespace GSM;
using namespace Control;



// The global transaction table.
TransactionTable gTransactionTable;

// The global TMSI table.
TMSITable gTMSITable;





TransactionEntry::TransactionEntry()
	:mID(gTransactionTable.newID()),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms),
	mTR1M(GSM::TR1Mms)
{
	mMessage[0]='\0';
}

TransactionEntry::TransactionEntry(const GSM::L3MobileIdentity& wSubscriber, 
	const GSM::L3CMServiceType& wService,
	const GSM::L3CallingPartyBCDNumber& wCalling)
	:mID(gTransactionTable.newID()),
	mSubscriber(wSubscriber),mService(wService),
	mTIFlag(1), mTIValue(0),
	mCalling(wCalling),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms),
	mTR1M(GSM::TR1Mms)
{
	mMessage[0]='\0';
}

TransactionEntry::TransactionEntry(const GSM::L3MobileIdentity& wSubscriber,
	const GSM::L3CMServiceType& wService,
	unsigned wTIValue,
	const GSM::L3CalledPartyBCDNumber& wCalled)
	:mID(gTransactionTable.newID()),
	mSubscriber(wSubscriber),mService(wService),
	mTIFlag(0), mTIValue(wTIValue),
	mCalled(wCalled),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms),
	mTR1M(GSM::TR1Mms)
{
	mMessage[0]='\0';
}

TransactionEntry::TransactionEntry(const GSM::L3MobileIdentity& wSubscriber,
	const GSM::L3CMServiceType& wService,
	unsigned wTIValue,
	const GSM::L3CallingPartyBCDNumber& wCalling)
	:mID(gTransactionTable.newID()),
	mSubscriber(wSubscriber),mService(wService),
	mTIValue(wTIValue),mCalling(wCalling),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms),
	mTR1M(GSM::TR1Mms)
{
	mMessage[0]='\0';
}



bool TransactionEntry::timerExpired() const
{
	// FIXME -- If we were smart, this would be a table.
	if (mT301.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T301 expired";
		return true;
	}
	if (mT302.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T302 expired";
		return true;
	}
	if (mT303.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T303 expired";
		return true;
	}
	if (mT304.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T304 expired";
		return true;
	}
	if (mT305.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T305 expired";
		return true;
	}
	if (mT308.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T308 expired";
		return true;
	}
	if (mT310.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T310 expired";
		return true;
	}
	if (mT313.expired()) {
		OBJLOG(DEBUG) << "TransactionEntry T313 expired";
		return true;
	}
	if (mTR1M.expired()) {
		OBJLOG(DEBUG) << "Transaction Entry TR1M expired";
		return true;
	}
	return false;
}


void TransactionEntry::resetTimers()
{
	mT301.reset();
	mT302.reset();
	mT303.reset();
	mT304.reset();
	mT305.reset();
	mT308.reset();
	mT310.reset();
	mT313.reset();
	mTR1M.reset();
}



bool TransactionEntry::dead() const
{
	if (mQ931State==NullState) return true;
	if ((mQ931State==Paging)&&mT3113.expired()) return true;
	return false;
}


ostream& Control::operator<<(ostream& os, TransactionEntry::Q931CallState state)
{
	switch (state) {
		case TransactionEntry::NullState: os << "null"; break;
		case TransactionEntry::Paging: os << "MTC paging"; break;
		case TransactionEntry::MOCInitiated: os << "MOC initiated"; break;
		case TransactionEntry::MOCProceeding: os << "MOC proceeding"; break;
		case TransactionEntry::MTCConfirmed: os << "MTC confirmed"; break;
		case TransactionEntry::CallReceived: os << "call received"; break;
		case TransactionEntry::CallPresent: os << "call present"; break;
		case TransactionEntry::ConnectIndication: os << "connect indication"; break;
		case TransactionEntry::Active: os << "active"; break;
		case TransactionEntry::DisconnectIndication: os << "disconnect indication"; break;
		case TransactionEntry::ReleaseRequest: os << "release request"; break;
		case TransactionEntry::SMSDelivering: os << "SMS delivery"; break;
		case TransactionEntry::SMSSubmitting: os << "SMS submission"; break;
		default: os << "?" << (int)state << "?";
	}
	return os;
}

ostream& Control::operator<<(ostream& os, const TransactionEntry& entry)
{
	os << "ID=" << entry.ID();
	os << " TI=(" << entry.TIFlag() << "," << entry.TIValue() << ") ";
	os << entry.subscriber();
	if (entry.called().digits()[0]) os << " to=" << entry.called().digits();
	if (entry.calling().digits()[0]) os << " from=" << entry.calling().digits();
	os << " Q.931State=" << entry.Q931State();
	os << " SIPState=" << entry.SIP().state();
	if (entry.message()[0]) os << " message=\"" << entry.message() << "\"";
	return os;
}


unsigned TransactionTable::newID()
{
	mLock.lock();
	unsigned ID = mIDCounter++;
	mLock.unlock();
	return ID;
}


void TransactionTable::add(const TransactionEntry& value)
{
	mLock.lock();
	mTable[value.ID()]=value;
	mLock.unlock();
}


void TransactionTable::update(const TransactionEntry& value)
{
	// ID==0 is a non-valid special case.
	assert(value.ID());
	mLock.lock();
	if (mTable.find(value.ID())==mTable.end()) {
		mLock.unlock();
		LOG(WARN) << "attempt to update non-existent transaction entry with key " << value.ID();
		return;
	}
	mTable[value.ID()]=value;
	mLock.unlock();
}




bool TransactionTable::find(unsigned key, TransactionEntry& target)
{
	// ID==0 is a non-valid special case.
	assert(key);
	mLock.lock();
	TransactionMap::iterator itr = mTable.find(key);
	if (itr==mTable.end()) {
		mLock.unlock();
		return false;
	}
	if (itr->second.dead()) {
		mTable.erase(itr);
		mLock.unlock();
		return false;
	}
	mLock.unlock();
	target = itr->second;
	return true;
}



bool TransactionTable::remove(unsigned key)
{
	// ID==0 is a non-valid special case.
	assert(key);
	mLock.lock();
	bool retVal = mTable.erase(key);
	mLock.unlock();
	return retVal;
}



void TransactionTable::clearDeadEntries()
{
	mLock.lock();
	TransactionMap::iterator itr = mTable.begin();
	while (itr!=mTable.end()) {
		if (!itr->second.dead()) ++itr;
		else {
			LOG(DEBUG) << "TransactionTable::clearDeadEntries erasing " << itr->first;
			TransactionMap::iterator old = itr;
			itr++;
			mTable.erase(old);
		}
	}
	mLock.unlock();
}



bool TransactionTable::find(const L3MobileIdentity& mobileID, TransactionEntry& target)
{
	// Yes, it's linear time.
	// Even in a 6-ARFCN system, it should rarely be more than a dozen entries.

	// Since clearDeadEntries is also linear, do that here, too.

	// Brtue force search.
	bool foundIt = false;
	mLock.lock();
	clearDeadEntries();
	TransactionMap::const_iterator itr = mTable.begin();
	while (itr!=mTable.end()) {
		const TransactionEntry& transaction = itr->second;
		if (transaction.subscriber()==mobileID) {
			// No need to check dead(), since we just cleared the table.
			foundIt = true;
			target = transaction;
			break;
		}
		++itr;
	}
	mLock.unlock();
	return foundIt;
}




void Control::clearTransactionHistory( TransactionEntry& transaction )
{
	SIP::SIPEngine& engine = transaction.SIP();
	LOG(DEBUG) << "clearTransactions "<<engine.callID()<<" "<< transaction.ID();
	gSIPInterface.removeCall(engine.callID());
	gTransactionTable.remove(transaction.ID());
}


void Control::clearTransactionHistory(unsigned transactionID)
{
	if (transactionID==0) return;
	TransactionEntry transaction;
	if (gTransactionTable.find(transactionID,transaction)) clearTransactionHistory(transaction);
}




unsigned TMSITable::assign(const char* IMSI)
{
	purge();
	mLock.lock();
	unsigned TMSI = mCounter++;
	mMap[TMSI] = IMSI;
	mLock.unlock();
	return TMSI;
}

const char* TMSITable::find(unsigned TMSI) const
{
	mLock.lock();
	TMSIMap::const_iterator iter = mMap.find(TMSI);
	mLock.unlock();
	if (iter==mMap.end()) return NULL;
	return iter->second.c_str();
}

unsigned TMSITable::find(const char* IMSI) const
{
	// FIXME -- If we were smart, we'd organize the table for a log-time search.
	unsigned TMSI = 0;
	string IMSIc(IMSI);
	mLock.lock();
	// brute force search
	TMSIMap::const_iterator itr = mMap.begin();
	while (itr!=mMap.end()) {
		if (itr->second == IMSIc) {
			TMSI = itr->first;
			break;
		}
		++itr;
	}
	mLock.unlock();
	return TMSI;
}




void TMSITable::erase(unsigned TMSI)
{
	mLock.lock();
	TMSIMap::iterator iter = mMap.find(TMSI);
	if (iter!=mMap.end()) mMap.erase(iter);
	mLock.unlock();
}


void TMSITable::purge()
{
	mLock.lock();
	// We rely on the fact the TMSIs are assigned in numeric order
	// to erase the oldest first.
	while ( (mMap.size()>mMaxSize) && (mClear!=mCounter) )
		mMap.erase(mClear++);
	mLock.unlock();
}





bool Control::waitForPrimitive(LogicalChannel *LCH, Primitive primitive, unsigned timeout_ms)
{
	bool waiting = true;
	while (waiting) {
		L3Frame *req = LCH->recv(timeout_ms);
		if (req==NULL) {
			LOG(NOTICE) << "(ControlLayer) waitForPrimitive timed out at uptime " << gBTS.uptime() << " frame " << gBTS.time();
			return false;
		}
		waiting = (req->primitive()!=primitive);
		delete req;
	}
	return true;
}


void Control::waitForPrimitive(LogicalChannel *LCH, Primitive primitive)
{
	bool waiting = true;
	while (waiting) {
		L3Frame *req = LCH->recv();
		if (req==NULL) continue;
		waiting = (req->primitive()!=primitive);
		delete req;
	}
}




// FIXME -- getMessage should return an L3Frame, not an L3Message.
// This will mean moving all of the parsing into the control layer.
// FIXME -- This needs an adjustable timeout.

L3Message* Control::getMessage(LogicalChannel *LCH, unsigned SAPI)
{
	unsigned timeout_ms = LCH->N200() * T200ms;
	L3Frame *rcv = LCH->recv(timeout_ms,SAPI);
	if (rcv==NULL) {
		LOG(NOTICE) << "getMessage timed out";
		throw ChannelReadTimeout();
	}
	LOG(DEBUG) << "getMessage got " << *rcv;
	Primitive primitive = rcv->primitive();
	if (primitive!=DATA) {
		LOG(NOTICE) << "getMessage got unexpected primitive " << primitive;
		delete rcv;
		throw UnexpectedPrimitive();
	}
	L3Message *msg = parseL3(*rcv);
	delete rcv;
	if (msg==NULL) {
		LOG(NOTICE) << "getMessage got unparsed message";
		throw UnsupportedMessage();
	}
	return msg;
}






/* Resolve a mobile ID to an IMSI and return TMSI if it is assigned. */
unsigned  Control::resolveIMSI(bool sameLAI, L3MobileIdentity& mobID, LogicalChannel* LCH)
{
	// Returns known or assigned TMSI.
	assert(LCH);
	LOG(DEBUG) << "resolving mobile ID " << mobID << ", sameLAI: " << sameLAI;

	// IMSI already?  See if there's a TMSI already, too.
	// This is a linear time operation, but should only happen on
	// the first registration by this mobile.
	if (mobID.type()==IMSIType) return gTMSITable.find(mobID.digits());

	// IMEI?  WTF?!
	// FIXME -- Should send MM Reject, cause 0x60, "invalid mandatory information".
	if (mobID.type()==IMEIType) throw UnexpectedMessage();

	// Must be a TMSI.
	// Look in the table to see if it's one we assigned.
	unsigned TMSI = mobID.TMSI();
	const char* IMSI = NULL;
	if (sameLAI) IMSI = gTMSITable.find(TMSI);
	if (IMSI) {
		// We assigned this TMSI and the TMSI/IMSI pair is already in the table.
		mobID = L3MobileIdentity(IMSI);
		LOG(DEBUG) << "resolving mobile ID (table): " << mobID;
		return TMSI;
	}
	// Not our TMSI.
	// Phones are not supposed to do this, but many will.
	// If the IMSI's not in the table, ASK for it.
	LCH->send(L3IdentityRequest(IMSIType));
	// FIXME -- This request times out on T3260, 12 sec.  See GSM 04.08 Table 11.2.
	L3Message* msg = getMessage(LCH);
	L3IdentityResponse *resp = dynamic_cast<L3IdentityResponse*>(msg);
	if (!resp) {
		if (msg) delete msg;
		throw UnexpectedMessage();
	}
	mobID = resp->mobileID();
	delete msg;
	LOG(DEBUG) << "resolving mobile ID (requested): " << mobID;
	if (mobID.type()!=IMSIType) throw UnexpectedMessage();
	// Return 0 to indicate that we have not yet assigned our own TMSI for this phone.
	return 0;
}



/* Resolve a mobile ID to an IMSI. */
void  Control::resolveIMSI(L3MobileIdentity& mobileIdentity, LogicalChannel* LCH)
{
	// Are we done already?
	if (mobileIdentity.type()==IMSIType) return;

	// If we got a TMSI, find the IMSI.
	if (mobileIdentity.type()==TMSIType) {
		const char *IMSI = gTMSITable.find(mobileIdentity.TMSI());
		if (IMSI) mobileIdentity = L3MobileIdentity(IMSI);
	}

	// Still no IMSI?  Ask for one.
	if (mobileIdentity.type()!=IMSIType) {
		LOG(NOTICE) << "MOC with no IMSI or valid TMSI.  Reqesting IMSI.";
		LCH->send(L3IdentityRequest(IMSIType));
		// FIXME -- This request times out on T3260, 12 sec.  See GSM 04.08 Table 11.2.
		L3Message* msg = getMessage(LCH);
		L3IdentityResponse *resp = dynamic_cast<L3IdentityResponse*>(msg);
		if (!resp) {
			if (msg) delete msg;
			throw UnexpectedMessage();
		}
		mobileIdentity = resp->mobileID();
		delete msg;
	}

	// Still no IMSI??
	if (mobileIdentity.type()!=IMSIType) {
		// FIXME -- This is quick-and-dirty, not following GSM 04.08 5.
		LOG(WARN) << "MOC setup with no IMSI";
		// Cause 0x60 "Invalid mandatory information"
		LCH->send(L3CMServiceReject(L3RejectCause(0x60)));
		LCH->send(L3ChannelRelease());
		// The SIP side and transaction record don't exist yet.
		// So we're done.
		return;
	}
}




// vim: ts=4 sw=4
