/**@file Declarations for all externally-visible control-layer functions. */
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
*/


#ifndef CONTROLCOMMON_H
#define CONTROLCOMMON_H

#define CLDCOUT(x) DCOUT("ControlLayer " << x)

#include <list>

#include "Interthread.h"
#include "Timeval.h"

/**
	Set the BTS' UDP port.
	Asterisk in on 5060, Zoiper on 5061 and the BTS on 5062.
*/
#define SIP_UDP_PORT 5062

/**
	We use a block of 100 ports (50 pairs) for RTP starting at this address.
*/
#define RTP_PORT_START 16484


#include "GSML3CommonElements.h"
#include "GSML3MMElements.h"
#include "GSML3CCElements.h"
#include "SIPEngine.h"


// Enough forward refs to prevent "kitchen sick" includes and circularity.

namespace GSM {
class Time;
class L3Message;
class GSMConfig;
class LogicalChannel;
class SDCCHLogicalChannel;
class CCCHLogicalChannel;
class TCHFACCHLogicalChannel;
class L3Cause;
class L3CMServiceRequest;
class L3LocationUpdatingRequest;
class L3IMSIDetachIndication;
class L3PagingResponse;
};


/**@namespace Control This namepace is for use by the control layer. */
namespace Control {

class TransactionEntry;
class TransactionTable;

/**@name Call control time-out values (in ms) from ITU-T Q.931 Table 9-1 and GSM 04.08 Table 11.4. */
//@{
#ifndef RACETEST
const unsigned T301ms=60000;		///< recv ALERT --> recv CONN
const unsigned T302ms=12000;		///< send SETUP ACK --> any progress
const unsigned T303ms=10000;			///< send SETUP --> recv CALL CONF or REL COMP
const unsigned T304ms=20000;		///< recv SETUP ACK --> any progress
const unsigned T305ms=30000;		///< send DISC --> recv REL or DISC
const unsigned T308ms=30000;		///< send REL --> rev REL or REL COMP
const unsigned T310ms=30000;		///< recv CALL CONF --> recv ALERT, CONN, or DISC
const unsigned T313ms=30000;		///< send CONNECT --> recv CONNECT ACK
#else
// These are reduced values to force testing of poor network behavior.
const unsigned T301ms=18000;		///< recv ALERT --> recv CONN
const unsigned T302ms=1200;		///< send SETUP ACK --> any progress
const unsigned T303ms=400;			///< send SETUP --> recv CALL CONF or REL COMP
const unsigned T304ms=2000;		///< recv SETUP ACK --> any progress
const unsigned T305ms=3000;		///< send DISC --> recv REL or DISC
const unsigned T308ms=3000;		///< send REL --> rev REL or REL COMP
const unsigned T310ms=3000;		///< recv CALL CONF --> recv ALERT, CONN, or DISC
const unsigned T313ms=3000;		///< send CONNECT --> recv CONNECT ACK
#endif
//@}


/**@name Common-use functions from the control layer. */
//@{

/**
	Common-use function to block on a channel until a given primitive arrives.
	Any payload is discarded.
	Does not throw excecptions.
	@return True on success, false on timeout.
*/
bool waitForPrimitive(GSM::LogicalChannel *LCH,
	GSM::Primitive primitive,
	unsigned timeout_ms=gBigReadTimeout);

/**
	Get a message from a LogicalChannel.
	Close the channel with abnormal release on timeout.
	Caller must delete the returned pointer.
	Throws ChannelReadTimeout, UnexpecedPrimitive or UnsupportedMessage on timeout.
	@param LCH The channel to receive on.
	@return Pointer to message.
*/
GSM::L3Message* getMessage(GSM::LogicalChannel* LCH);

/**
	Clear the state information associated with a TransactionEntry.
	Removes the transaction data from both gSIPInterfaceMap and gTransactionTable.
	@param transaction The transaction to clear.
*/
void clearTransactionHistory(TransactionEntry& transaction);

/**
	Clear the state information associated with a TransactionEntry.
	Removes the transaction data from both gSIPInterfaceMap and gTransactionTable.
	@param transactionID The ID of the transaction.
*/
void clearTransactionHistory(unsigned transactionID);

//@}



/**@name Functions for mobility manangement operations. */
//@{
void CMServiceResponder(const GSM::L3CMServiceRequest* cmsrq, GSM::SDCCHLogicalChannel* SDCCH);
void IMSIDetachController(const GSM::L3IMSIDetachIndication* idi, GSM::SDCCHLogicalChannel* SDCCH);
void LocationUpdatingController(const GSM::L3LocationUpdatingRequest* lur, GSM::SDCCHLogicalChannel* SDCCH);
//@}

/**@name Functions for radio resource operations. */
//@{
void AccessGrantResponder(unsigned requestReference, const GSM::Time& when);
void PagingResponseHandler(const GSM::L3PagingResponse*, GSM::SDCCHLogicalChannel*);
//@}

/**@name Functions for call control operations. */
//@{
/**@name MOC */
//@{
void MOCStarter(const GSM::L3CMServiceRequest* req, GSM::SDCCHLogicalChannel *SDCCH);
void MOCController(TransactionEntry& transaction, GSM::TCHFACCHLogicalChannel* TCHFACCH);
//@}
/**@name MTC */
//@{
void MTCStarter(const GSM::L3PagingResponse *resp, GSM::SDCCHLogicalChannel *SDCCH);
void MTCController(TransactionEntry& transaction, GSM::TCHFACCHLogicalChannel* TCHFACCH);
//@}
/**@name MOSMS */
//@{
void ShortMessageServiceStarter(const GSM::L3CMServiceRequest*resp, 
						GSM::SDCCHLogicalChannel *SDCCH);
//@}




/**@name Abnormal clearing */
//@{
void forceGSMClearing(TransactionEntry& transaction, GSM::LogicalChannel *LCH, const GSM::L3Cause& cause);
void forceSIPClearing(TransactionEntry& transaction);
void abortCall(TransactionEntry& transaction, GSM::LogicalChannel *LCH, const GSM::L3Cause& cause);
//@}
//@}

/**@name Dispatch controllers for specific channel types. */
//@{
void FACCHDispatcher(GSM::TCHFACCHLogicalChannel *TCHFACCH);
void SDCCHDispatcher(GSM::SDCCHLogicalChannel *SDCCH);
//@}






/** An entry in the paging list. */
class PagingEntry {

	private:

	GSM::L3MobileIdentity mID;		///< The mobile ID.
	Timeval mExpiration;			///< The expiration time for this entry.

	public:

	/**
		Create a new entry, with current timestamp.
		@param wID The ID to be paged.
		@param wLife The number of milliseconds to keep paging.
	*/
	PagingEntry(const GSM::L3MobileIdentity& wID, unsigned wLife)
		:mID(wID),mExpiration(wLife)
	{}

	/** Access the ID. */
	const GSM::L3MobileIdentity& ID() const { return mID; }

	/** Renew the timer. */
	void renew(unsigned wLife) { mExpiration = Timeval(wLife); }

	/** Returns true if the entry is expired. */
	bool expired() const { return mExpiration.passed(); }

};


/**
	The pager is a global object that generates paging messages on the CCCH.
	To page a mobile, add the mobile ID to the pager.
	The entry will be deleted automatically when it expires.
*/
class Pager {

	private:

	std::list<PagingEntry> mPageIDs;		///< List of ID's to be paged.
	Mutex mLock;							///< Lock for thread-safe access.
	Signal mPageSignal;						///< signal to wake the paging loop
	Thread mPagingThread;					///< Thread for the paging loop.
	volatile bool mRunning;

	public:

	Pager()
		:mRunning(false)
	{}

	/** Set the output FIFO and start the paging loop. */
	void start();

	/**
		Add a mobile ID to the paging list.
		@param addID The mobile ID to be paged.
		@param wLife The paging duration in ms, default based on SIP INVITE retry preiod.
	*/
	void addID(const GSM::L3MobileIdentity& addID, unsigned wLife=4000);

	private:

	/**
		Traverse the paging list, paging all IDs.
		@return Number of IDs paged.
	*/
	unsigned pageAll();

	/** A loop that repeatedly calls pageAll. */
	void serviceLoop();

	/** C-style adapter. */
	friend void *PagerServiceLoopAdapter(Pager*);
};


void *PagerServiceLoopAdapter(Pager*);




/**
	A TransactionEntry object is used to maintain the state of a transaction
	as it moves from channel to channel.
	The object itself is not thread safe.
*/
class TransactionEntry {

	public:

	/** Call states based on GSM 04.08 5 and ITU-T Q.931 */
	enum Q931CallState {
		NullState,
		Paging,
		MOCInitiated,
		MOCProceeding,
		MTCConfirmed,
		CallReceived,
		CallPresent,
		ConnectIndication,
		Active,
		DisconnectIndication,
		ReleaseRequest
	};

	private:

	unsigned mID;								///< the internal transaction ID, assigned by a TransactionTable

	GSM::L3MobileIdentity mSubscriber;		///< some kind of subscriber ID, preferably IMSI
	GSM::L3CMServiceType mService;			///< the associated service type
	unsigned mTIFlag;							///< "0" for originating party ,"1" for terminating
	unsigned mTIValue;							///< the L3 short transaction ID set by the MS
	GSM::L3CalledPartyBCDNumber mCalled;	///< the associated called party number, if known
	GSM::L3CallingPartyBCDNumber mCalling;	///< the associated calling party number, if known

	SIP::SIPEngine mSIP;						///< the SIP IETF RFC-3621 protocol engine
	Q931CallState mQ931State;					///< the GSM/ISDN/Q.931 call state

	/**@name Timers from GSM and Q.931 (network side) */
	//@{
	// If you add a timer, remember to add it to
	// the constructor, timerExpired and resetTimers methods.
	GSM::Z100Timer mT301;
	GSM::Z100Timer mT302;
	GSM::Z100Timer mT303;
	GSM::Z100Timer mT304;
	GSM::Z100Timer mT305;		///< a "clearing timer"
	GSM::Z100Timer mT308;		///< a "clearing timer"
	GSM::Z100Timer mT310;
	GSM::Z100Timer mT313;
	GSM::Z100Timer mT3113;		///< the paging timer, NOT a Q.931 timer
	//@}

	public:

	TransactionEntry()
		:mID(0),mQ931State(NullState),
		mT301(T301ms), mT302(T302ms), mT303(T303ms),
		mT304(T304ms), mT305(T305ms), mT308(T308ms),
		mT310(T310ms), mT313(T313ms),
		mT3113(GSM::T3113ms)
	{}

	/** This form is used for MTC. */
	TransactionEntry(const GSM::L3MobileIdentity& wSubscriber, 
		const GSM::L3CMServiceType& wService,
		const GSM::L3CallingPartyBCDNumber& wCalling)
		:mID(0),mSubscriber(wSubscriber),mService(wService),
		// TIValue=7 means non-valid TI.
		mTIFlag(1), mTIValue(7),
		mCalling(wCalling),
		mSIP(SIP_UDP_PORT,5060,"127.0.0.1"),
		mQ931State(NullState),
		mT301(T301ms), mT302(T302ms), mT303(T303ms),
		mT304(T304ms), mT305(T305ms), mT308(T308ms),
		mT310(T310ms), mT313(T313ms),
		mT3113(GSM::T3113ms)
	{}

	/** This form is used for MOC. */
	TransactionEntry(const GSM::L3MobileIdentity& wSubscriber,
		const GSM::L3CMServiceType& wService,
		unsigned wTIValue,
		const GSM::L3CalledPartyBCDNumber& wCalled)
		:mID(0),mSubscriber(wSubscriber),mService(wService),
		mTIFlag(0), mTIValue(wTIValue),
		mCalled(wCalled),
		mSIP(SIP_UDP_PORT,5060,"127.0.0.1"),
		mQ931State(NullState),
		mT301(T301ms), mT302(T302ms), mT303(T303ms),
		mT304(T304ms), mT305(T305ms), mT308(T308ms),
		mT310(T310ms), mT313(T313ms),
		mT3113(GSM::T3113ms)
	{}

	TransactionEntry(const GSM::L3MobileIdentity& wSubscriber,
		const GSM::L3CMServiceType& wService,
		unsigned wTIValue,
		const GSM::L3CallingPartyBCDNumber& wCalling)
		:mID(0),mSubscriber(wSubscriber),mService(wService),
		mTIValue(wTIValue),mCalling(wCalling),
		mSIP(SIP_UDP_PORT,5060,"127.0.0.1"),
		mQ931State(NullState),
		mT301(T301ms), mT302(T302ms), mT303(T303ms),
		mT304(T304ms), mT305(T305ms), mT308(T308ms),
		mT310(T310ms), mT313(T313ms),
		mT3113(GSM::T3113ms)
	{}

	/**@name Accessors. */
	//@{
	unsigned TIValue() const { return mTIValue; }
	void TIValue(unsigned wTIValue) { mTIValue = wTIValue; }

	unsigned TIFlag() const { return mTIFlag; }

	const GSM::L3MobileIdentity& subscriber() const { return mSubscriber; }

	const GSM::L3CMServiceType& service() const { return mService; }

	const GSM::L3CalledPartyBCDNumber& called() const { return mCalled; }

	const GSM::L3CallingPartyBCDNumber& calling() const { return mCalling; }

	unsigned ID() const { return mID; }

	SIP::SIPEngine& SIP() { return mSIP; }

	void Q931State(Q931CallState wState) { mQ931State=wState; }
	Q931CallState Q931State() const { return mQ931State; }

	/**@name Timer access. */
	// TODO -- If we were clever, this would be a table.
	//@{
	GSM::Z100Timer& T301() { return mT301; }
	GSM::Z100Timer& T302() { return mT302; }
	GSM::Z100Timer& T303() { return mT303; }
	GSM::Z100Timer& T304() { return mT304; }
	GSM::Z100Timer& T305() { return mT305; }
	GSM::Z100Timer& T308() { return mT308; }
	GSM::Z100Timer& T310() { return mT310; }
	GSM::Z100Timer& T313() { return mT313; }
	GSM::Z100Timer& T3113() { return mT3113; }
	//@}
	//@}

	/** Return true if clearing is in progress. */
	bool clearing() const
		{ return (mQ931State==ReleaseRequest) || (mQ931State==DisconnectIndication); }

	/** Return true if any Q.931 timer is expired. */
	bool timerExpired() const;

	/** Reset all Q.931 timers. */
	void resetTimers();

	/** Retrns true if the transaction is "dead". */
	bool dead() const
		{ return ((mQ931State==Paging)&&mT3113.expired()) || (mQ931State==NullState); }

	private:

	friend class TransactionTable;

	void ID(unsigned wID) { mID=wID; }
};


std::ostream& operator<<(std::ostream& os, TransactionEntry::Q931CallState);


/** A map of transactions keyed by ID. */
class TransactionMap : public std::map<unsigned,TransactionEntry> {};

/**
	A table for tracking the states of active transactions.
	Note that transaction table add and find operations
	are pass-by-copy, not pass-by-reference.
*/
class TransactionTable {

	private:

	TransactionMap mTable;
	mutable Mutex mLock;
	unsigned mIDCounter;

	public:

	TransactionTable()
	{
		// Initialize mIDCounter with a random value.
		mIDCounter = random();
	}

	/**
		Insert a new entry into the table.
		Also assigns a transaction ID to the argument.
		@param value The entry to copy into the table.
		@return The assigned transaction ID.
	*/
	unsigned add(TransactionEntry& value);

	/**
		Update a transaction in the table.
		Uses the ID previously assigned to the TransactionEntry by add().
		@param value The new TransactionEntry to be copied in.
	*/
	void update(const TransactionEntry& value);

	/**
		Find an entry and make a copy.
		@param wID The transaction ID to search.
		@param target A place to put the copy.
		@return True if successful.
	*/
	bool find(unsigned wID, TransactionEntry& target) const;

	/**
		Remove an entry from the table.
		@param wID The transaction ID to search.
		@return True if the ID was really in the table.
	*/
	bool remove(unsigned wID);

	/**
		Find an entry by its mobile ID.
		@param mobileID The mobile at to search for.
		@param target A TransactionEntry to accept the found record.
		@return Entry transaction ID or 0 on failure.
	*/
	unsigned findByMobileID(const GSM::L3MobileIdentity& mobileID, TransactionEntry& target) const;

	/**
		Remove "dead" entries from the table.
	*/
	void clearDeadEntries();
};



/**@name Control-layer exceptions. */
//@{

/**
	A control layer excpection includes a pointer to a transaction.
	The transaction might require some clean-up action, depending on the exception.
*/
class ControlLayerException {

	private:

	unsigned mTransactionID;

	public:

	ControlLayerException(unsigned wTransactionID=0)
		:mTransactionID(wTransactionID)
	{}

	unsigned transactionID() { return mTransactionID; }
};

/** Thrown when the control layer gets the wrong message */
class UnexpectedMessage : public ControlLayerException {
	public:
	UnexpectedMessage(unsigned wTransactionID=0)
		:ControlLayerException(wTransactionID)
	{}
};

/** Thrown when recvL3 returns NULL */
class ChannelReadTimeout : public ControlLayerException {
	public:
	ChannelReadTimeout(unsigned wTransactionID=0)
		:ControlLayerException(wTransactionID)
	{}
};

/** Thrown when L3 can't parse an incoming message */
class UnsupportedMessage : public ControlLayerException {
	public:
	UnsupportedMessage(unsigned wTransactionID=0)
		:ControlLayerException(wTransactionID)
	{}
};

/** Thrown when the control layer gets the wrong primitive */
class UnexpectedPrimitive : public ControlLayerException {
	public:
	UnexpectedPrimitive(unsigned wTransactionID=0)
		:ControlLayerException(wTransactionID)
	{}
};

/**  Thrown when a T3xx expires */
class Q931TimerExpired : public ControlLayerException {
	public:
	Q931TimerExpired(unsigned wTransactionID=0)
		:ControlLayerException(wTransactionID)
	{}
};


//@}




}	//Control


/**@addtogroup Globals */
//@{
/** A single global transaction table in the global namespace. */
extern Control::TransactionTable gTransactionTable;
//@}



#endif

// vim: ts=4 sw=4
