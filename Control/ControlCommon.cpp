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
	mT3113(GSM::T3113ms)
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
	mSIP(),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms)
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
	mSIP(),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms)
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
	mSIP(),
	mQ931State(NullState),
	mT301(T301ms), mT302(T302ms), mT303(T303ms),
	mT304(T304ms), mT305(T305ms), mT308(T308ms),
	mT310(T310ms), mT313(T313ms),
	mT3113(GSM::T3113ms)
{
	mMessage[0]='\0';
}



bool TransactionEntry::timerExpired() const
{
	if (mT301.expired()) {
		OBJDCOUT("TransactionEntry T301 expired");
		return true;
	}
	if (mT302.expired()) {
		OBJDCOUT("TransactionEntry T302 expired");
		return true;
	}
	if (mT303.expired()) {
		OBJDCOUT("TransactionEntry T303 expired");
		return true;
	}
	if (mT304.expired()) {
		OBJDCOUT("TransactionEntry T304 expired");
		return true;
	}
	if (mT305.expired()) {
		OBJDCOUT("TransactionEntry T305 expired");
		return true;
	}
	if (mT308.expired()) {
		OBJDCOUT("TransactionEntry T308 expired");
		return true;
	}
	if (mT310.expired()) {
		OBJDCOUT("TransactionEntry T310 expired");
		return true;
	}
	if (mT313.expired()) {
		OBJDCOUT("TransactionEntry T313 expired");
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
		default: os << "?" << (int)state << "?";
	}
	return os;
}

ostream& Control::operator<<(ostream& os, const TransactionEntry& entry)
{
	os << "ID=" << entry.ID();
	os << " TI=(" << entry.TIFlag() << "," << entry.TIValue() << ")";
	os << " Q.931State=" << entry.Q931State();
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
	clearDeadEntries();
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
		CERR("WARNING -- attempt to update non-existent transaction entry with key " << value.ID());
		return;
	}
	mTable[value.ID()]=value;
	mLock.unlock();
}




bool TransactionTable::find(unsigned key, TransactionEntry& target) const
{
	// ID==0 is a non-valid special case.
	assert(key);
	bool retVal=false;
	mLock.lock();
	TransactionMap::const_iterator itr = mTable.find(key);
	if (itr!=mTable.end()) {
		if (!itr->second.dead()) {
			target = itr->second;
			retVal=true;
		}
	}
	mLock.unlock();
	return retVal;
}



bool TransactionTable::remove(unsigned key)
{
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
			CLDCOUT("TransactionTable::clearDeadEntries erasing " << itr->first);
			TransactionMap::iterator old = itr;
			itr++;
			mTable.erase(old);
		}
	}
	mLock.unlock();
}



bool TransactionTable::find(const L3MobileIdentity& mobileID, TransactionEntry& target) const
{
	// Yes, it's linear time.
	// Even in a 6-ARFCN system, it should rarely be more than a dozen entries.

	// Brtue force search.
	bool foundIt = false;
	mLock.lock();
	TransactionMap::const_iterator itr = mTable.begin();
	while (itr!=mTable.end()) {
		const TransactionEntry& transaction = itr->second;
		if (transaction.subscriber()==mobileID) {
			if (!transaction.dead()) {
				foundIt = true;
				target = transaction;
			}
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
	CLDCOUT("clearTransactions "<<engine.callID()<<" "<< transaction.ID())
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
			CERR("NOTICE -- (ControlLayer) waitForPrimitive timed out at uptime " << gBTS.uptime() << " frame " << gBTS.time());
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
	//unsigned timeout_ms = LCH->N200() * T200ms;
	L3Frame *rcv = LCH->recv(LCH->N200()*T200ms,SAPI);
	if (rcv==NULL) {
		CERR("NOTICE -- getMessage timed out");
		throw ChannelReadTimeout();
	}
	CLDCOUT("getMessage got " << *rcv);
	Primitive primitive = rcv->primitive();
	if (primitive!=DATA) {
		CERR("NOTICE -- getMessage got unexpected primitive " << primitive);
		delete rcv;
		throw UnexpectedPrimitive();
	}
	L3Message *msg = parseL3(*rcv);
	delete rcv;
	if (msg==NULL) {
		CERR("NOTICE -- getMessage got unparsed message");
		throw UnsupportedMessage();
	}
	return msg;
}





/**
	Force clearing on the GSM side.
	@param transaction The call transaction record.
	@param LCH The logical channel.
	@param cause The L3 abort cause.
*/
void Control::forceGSMClearing(TransactionEntry& transaction, LogicalChannel *LCH, const L3Cause& cause)
{
	if (transaction.Q931State()==TransactionEntry::NullState) return;
	CLDCOUT("forceGSMClearing from call state " << transaction.Q931State());
	if (!transaction.clearing()) {
		LCH->send(L3Disconnect(1-transaction.TIFlag(),transaction.TIValue(),cause));
	}
	LCH->send(L3ReleaseComplete(0,transaction.TIValue()));
	LCH->send(L3ChannelRelease());
	transaction.resetTimers();
	transaction.Q931State(TransactionEntry::NullState);
	LCH->send(RELEASE);
}


/**
	Force clearing on the SIP side.
	@param transaction The call transaction record.
*/
void Control::forceSIPClearing(TransactionEntry& transaction)
{
	if (transaction.SIP().state()==SIP::Cleared) return;
	CLDCOUT("forceSIPClearing from call state " << transaction.SIP().state());
	if (transaction.SIP().state()!=SIP::Clearing) {
		transaction.SIP().MODSendBYE();
	} else {
		transaction.SIP().MODResendBYE();
	}
	// FIXME -- We need to loop here and wait for the OK or timeout from the SIP side.
}



/**
	Abort the call.
	@param transaction The call transaction record.
	@param LCH The logical channel.
	@param cause The L3 abort cause.
*/
void Control::abortCall(TransactionEntry& transaction, LogicalChannel *LCH, const L3Cause& cause)
{
	CLDCOUT("abortCall transction: " << transaction);
	forceGSMClearing(transaction,LCH,cause);
	forceSIPClearing(transaction);
	gTransactionTable.update(transaction);
}




/* Resolve a mobile ID to an IMSI. */
unsigned  Control::resolveIMSI(bool sameLAI, L3MobileIdentity& mobID, LogicalChannel* LCH)
{
	assert(LCH);
	CLDCOUT("resolving mobile ID " << mobID << ", sameLAI: " << sameLAI);

	// IMSI already?  See if there's a TMSI already, too.
	// This is a linear time operation, but should only happen on
	// the first registration by this mobile.
	if (mobID.type()==IMSIType) return gTMSITable.find(mobID.digits());

	// IMEI?  WTF?!
	if (mobID.type()==IMEIType) throw UnexpectedMessage();

	// Must be a TMSI.
	// Look in the table to see if it's one we assigned.
	unsigned TMSI = mobID.TMSI();
	const char* IMSI = NULL;
	if (sameLAI) IMSI = gTMSITable.find(TMSI);
	if (IMSI) {
		// We assigned this TMSI and the TMSI/IMSI pair is already in the table.
		mobID = L3MobileIdentity(IMSI);
		CLDCOUT("resolving mobile ID (table): " << mobID);
		return TMSI;
	}
	// Not our TMSI.
	// Phones are not supposed to do this, but many will.
	// If the IMSI's not in the table, ASK for it.
	LCH->send(L3IdentityRequest(IMSIType));
	// FIXME -- This request times out on T3260, 12 sec.  See GSM 04.08 Table 11.2.
	L3IdentityResponse *resp = dynamic_cast<L3IdentityResponse*>(getMessage(LCH));
	if (!resp) throw UnexpectedMessage();
	mobID = resp->mobileID();
	delete resp;
	CLDCOUT("resolving mobile ID (requested): " << mobID);
	if (mobID.type()!=IMSIType) throw UnexpectedMessage();
	// Return 0 to indicate that we have not yet assigned our own TMSI for this phone.
	return 0;
}





// vim: ts=4 sw=4
