/**@file GSM/SIP Call Control -- GSM 04.08, ISDN ITU-T Q.931, SIP IETF RFC-3261, RTP IETF RFC-3550. */
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


/*
	Abbreviations:
	MTC -- Mobile Terminated Connect (someone calling the mobile)
	MOC -- Mobile Originated Connect (mobile calling out)
	MTD -- Mobile Terminated Disconnect (other party hangs up)
	MOD -- Mobile Originated Disconnect (mobile hangs up)
*/


#include <Globals.h>

#include "ControlCommon.h"

#include <GSMLogicalChannel.h>
#include <GSML3RRMessages.h>
#include <GSML3MMMessages.h>
#include <GSML3CCMessages.h>
#include <GSMConfig.h>

#include <SIPInterface.h>
#include <SIPUtility.h>
#include <SIPMessage.h>
#include <SIPEngine.h>

using namespace std;
using namespace GSM;
using namespace Control;
using namespace SIP;






/**
	Return an even UDP port number for the RTP even/odd pair.
*/
unsigned allocateRTPPorts()
{
	// FIXME -- We need a real port allocator (bug #82).
	const unsigned base = gConfig.getNum("RTP.Start");
	const unsigned range = gConfig.getNum("RTP.Range");
	const unsigned top = base+range;
	static Mutex lock;
	// Pick a random starting point.
	static unsigned port = base + 2*(random()%(range/2));
	lock.lock();
	unsigned retVal = port;
	port += 2;
	if (port>=top) port=base;
	lock.unlock();
	return retVal;
}




/**
	Allocate a TCH and clean up any failure.
	@param SDCCH The SDCCH that will be used to send the assignment.
	@return A pointer to the TCH or NULL on failure.
*/
TCHFACCHLogicalChannel *allocateTCH(SDCCHLogicalChannel *SDCCH)
{
	TCHFACCHLogicalChannel *TCH = gBTS.getTCH();
	if (TCH==NULL) {
		CERR("NOTICE -- (ControlLayer) no TCH available for assignment");
		CLDCOUT("allocateTCH: CONGESTION");
		// Cause 0x16 is "congestion".
		SDCCH->send(L3CMServiceReject(0x16));
		SDCCH->send(L3ChannelRelease());
	}
	//if (TCH!=NULL) CLDCOUT("allocateTCH allocates (L2) " << TCH->debugGetL2Proc());
	return TCH;
}





/**
	Assign a full rate traffic channel and clean up any failures.
	@param SDCCH The SDCCH on which to send the assignment.
	@param TCH The TCH to be assigned.
	@bool True on successful transfer.
*/
bool assignTCHF(SDCCHLogicalChannel *SDCCH, TCHFACCHLogicalChannel *TCH)
{
	TCH->open();

	// Try twice to send the assignment.
	// (The spec says just try once...)
	for (int i=0; i<2; i++) {
		CLDCOUT("assignTCHF sending AssignmentCommand for " << TCH << " on " << SDCCH);
		SDCCH->send(L3AssignmentCommand(TCH->channelDescription(),L3ChannelMode(L3ChannelMode::SpeechV1)));
	
		// This read is SUPPOSED to time out if the assignment was successful.
		// Pad the timeout just in case there's a large latency somewhere.
		L3Frame *result = SDCCH->recv(T3107ms+2000);
		if (result==NULL) {
			CLDCOUT("assignmentTCHF exiting normally");
			SDCCH->send(RELEASE);
			return true;
		}

		CLDCOUT("assignTCHF received " << *result);
		delete result;
	}

	// If we got here, the assignment failed.
	TCH->send(RELEASE);
	// RR Cause 0x04 -- "abnormal release, no activity on the radio path"
	SDCCH->send(L3ChannelRelease(0x04));
	return false;
}




/**
	Process a message received from the phone during a call.
	This function processes all deviations from the "call connected" state.
	For now, we handle call clearing and politely reject everything else.
	@param transaction The transaction record for this call.
	@param LCH The logical channel for the transaction.
	@param message A pointer to the receiver message.
	@return true If the call has been cleared and the channel released.
*/
bool callManagementDispatchGSM(TransactionEntry& transaction, LogicalChannel* LCH, const L3Message *message)
{
	CLDCOUT("callManagementDispatchGSM " << *message);

	// FIXME -- This dispatch section should be something more efficient with PD and MTI swtiches.
	// FIXME -- Actually check state before taking action.

	// Call connection steps.

	// Connect Acknowledge
	if (dynamic_cast<const L3ConnectAcknowledge*>(message)) {
		CLDCOUT("received GSM Connect Acknowledge");
		transaction.resetTimers();
		transaction.Q931State(TransactionEntry::Active);
		return false;
	}

	// Connect
	// GSM 04.08 5.2.2.5 and 5.2.2.6
	if (dynamic_cast<const L3Connect*>(message)) {
		CLDCOUT("received GSM Connect");
		transaction.resetTimers();
		transaction.Q931State(TransactionEntry::Active);
		return false;
	}

	// Call Confirmed
	// GSM 04.08 5.2.2.3.2
	// "Call Confirmed" is the GSM MTC counterpart to "Call Proceeding"
	if (dynamic_cast<const L3CallConfirmed*>(message)) {
		CLDCOUT("received GSM Call Confirmed");
		transaction.T303().reset();
		transaction.T310().set();
		transaction.Q931State(TransactionEntry::MTCConfirmed);
		return false;
	}

	// Alerting
	// GSM 04.08 5.2.2.3.2
	if (dynamic_cast<const L3Alerting*>(message)) {
		CLDCOUT("received GSM Alerting");
		transaction.T310().reset();
		transaction.T301().set();
		transaction.Q931State(TransactionEntry::CallReceived);
		return false;
	}

	// Call clearing steps.
	// Good diagrams in GSM 04.08 7.3.4

	// FIXME -- We should be checking TI values against the transaction object.

	// Disconnect (1st step of MOD)
	// GSM 04.08 5.4.3.2
	if (const L3Disconnect* disconnect = dynamic_cast<const L3Disconnect*>(message)) {
		CLDCOUT("received GSM Disconnect");
		unsigned L3TI = disconnect->TIValue();
		unsigned L3Flag = 1 - disconnect->TIFlag();
		transaction.resetTimers();
		LCH->send(L3Release(L3Flag,L3TI));
		transaction.T308().set();
		transaction.Q931State(TransactionEntry::ReleaseRequest);
		transaction.SIP().MODSendBYE();
		return false;
	}

	// Release (2nd step of MTD)
	if (const L3Release* release = dynamic_cast<const L3Release*>(message)) {
		CLDCOUT("received GSM Release");
		unsigned L3TI = release->TIValue();
		unsigned L3Flag = 1 - release->TIFlag();
		transaction.resetTimers();
		LCH->send(L3ReleaseComplete(L3Flag,L3TI));
		LCH->send(L3ChannelRelease());
		transaction.Q931State(TransactionEntry::NullState);
		transaction.SIP().MTDSendOK();
		return true;
	}

	// Release Complete (3nd step of MOD)
	// GSM 04.08 5.4.3.4
	if (dynamic_cast<const L3ReleaseComplete*>(message)) {
		CLDCOUT("received GSM Release Complete");
		transaction.resetTimers();
		LCH->send(L3ChannelRelease());
		transaction.Q931State(TransactionEntry::NullState);
		forceSIPClearing(transaction);
		return true;
	}

	// Stubs for unsupported features.

	// CM Service Request
	// This is the gateway to a much more complex state machine.
	// For now, we're cutting it off right here.
	if (dynamic_cast<const L3CMServiceRequest*>(message)) {
		// Cause 0x20 means "serivce not supported".
		LCH->send(L3CMServiceReject(0x20));
		return false;
	}

	// Start DTMF
	// Send a SIP INFO to generate a tone in Asterisk.
	if (const L3StartDTMF* keypress = dynamic_cast<const L3StartDTMF*>(message)) {
		// Cause 0x3f means "service or option not available".
		unsigned L3TI = keypress->TIValue();
		unsigned keyVal = encodeBCDChar(keypress->key().IA5());
		bool success = transaction.SIP().sendINFOAndWaitForOK(keyVal);
		if (success) LCH->send(L3StartDTMFAcknowledge(1,L3TI,keypress->key()));
		else LCH->send(L3StartDTMFReject(1,L3TI,0x3f));
		return false;
	}

	// Stop DTMF
	// Since we use SIP INFO we just ack.
	if (const L3StopDTMF* stop = dynamic_cast<const L3StopDTMF*>(message)) {
		// Cause 0x3f means "service or option not available".
		unsigned L3TI = stop->TIValue();
		LCH->send(L3StopDTMFAcknowledge(1,L3TI));
		return false;
	}


	// If we got here, we're ignoring the message.
	return false;
}






/**
	Update vocoder data transfers in both directions.
	@param transaction The transaction object for this call.
	@param TCH The traffic channel for this call.
	@return True if anything was transferred.
*/
bool updateCallTraffic(TransactionEntry &transaction, TCHFACCHLogicalChannel *TCH)
{
	bool activity = false;

	SIPEngine& engine = transaction.SIP();


	// Transfer in the downlink direction (RTP->GSM).
	// Blocking call.  On average returns 1 time per 20 ms.
	// Returns non-zero if anything really happened.
	// Make the rxFrame buffer big enough for G.711.
	unsigned char rxFrame[160];
	if (engine.RxFrame(rxFrame)) {
		activity = true;
		TCH->sendTCH(rxFrame);
	}

	// Transfer in the uplink direction (GSM->RTP).
	// Flush FIFO to limit latency.
	while (TCH->queueSize()>2) delete TCH->recvTCH();
	if (unsigned char *txFrame = TCH->recvTCH()) {
		activity = true;
		// Send on RTP.
		engine.TxFrame(txFrame);
		delete txFrame;
	}

	// Return a flag so the caller will know if anything transferred.
	return activity;
}




/**
	Check GSM signalling.
	Can block for up to 52 GSM L1 frames (240 ms) because LCH::send is blocking.
	@param transaction The call's TransactionEntry.
	@param LCH The call's logical channel (TCH/FACCH or SDCCH).
	@return true If the call was cleared.
*/
bool updateGSMSignalling(TransactionEntry &transaction, LogicalChannel *LCH, unsigned timeout=0)
{
	if (transaction.Q931State()==TransactionEntry::NullState) return true;

	// Any Q.931 timer expired?
	if (transaction.timerExpired()) {
		// Cause 0x66, "recover on timer expiry"
		abortCall(transaction,LCH,L3Cause(0x66));
		return true;
	}

	// Look for a control message from MS side.
	if (L3Frame *l3 = LCH->recv(timeout)) {
		L3Message *l3msg = parseL3(*l3);
		delete l3;
		bool cleared = false;
		if (l3msg) {
			CLDCOUT("callManagementLoop got GSM " << *l3msg);
			cleared = callManagementDispatchGSM(transaction, LCH, l3msg);
			delete l3msg;
		}
		return cleared;
	}

	// If we are here, we have timed out, but assume the call is still running.
	return false;
}



/**
	Check SIP and GSM signalling.
	Can block for up to 52 GSM L1 frames (240 ms) because LCH::send is blocking.
	@param transaction The call's TransactionEntry.
	@param LCH The call's logical channel (TCH/FACCH or SDCCH).
	@return true If the call is cleared in both domains.
*/
bool updateSignalling(TransactionEntry &transaction, LogicalChannel *LCH, unsigned timeout=0)
{

	bool GSMCleared = (updateGSMSignalling(transaction,LCH,timeout));

	// Look for a SIP message.
	SIPEngine& engine = transaction.SIP();
	if (engine.MTDCheckBYE() == SIP::Clearing) {
		if (!transaction.clearing()) {
			CLDCOUT("callManagementLoop got BYE");
			LCH->send(L3Disconnect(1-transaction.TIFlag(),transaction.TIValue()));
			transaction.T305().set();
			transaction.Q931State(TransactionEntry::DisconnectIndication);
			// Return false, because it the call is not yet cleared.
			return false;
		} else {
			// If we're already clearing, just ack with OK.
			engine.MTDSendOK();
		}
	}
	bool SIPCleared = (engine.state()==SIP::Cleared);

	return GSMCleared && SIPCleared;
}




/**
	Poll for activity while in a call.
	Sleep if needed to prevent fast spinning.
	Will block for up to 250 ms.
	@param transaction The call's TransactionEntry.
	@param TCH The call's TCH+FACCH.
	@return true If the call was cleared.
*/
bool pollInCall(TransactionEntry &transaction, TCHFACCHLogicalChannel *TCH)
{
	// See if the radio link disappeared.
	if (TCH->radioFailure()) {
		forceSIPClearing(transaction);
		return true;
	}
	// Process pending SIP and GSM signalling.
	// If this returns true, it means the call is fully cleared.
	if (updateSignalling(transaction,TCH)) return true;
	// Transfer vocoder data.
	// If anything happened, then the call is still up.
	if (updateCallTraffic(transaction,TCH)) return false;
	// If nothing happened, sleep so we don't burn up the CPU cycles.
	msleep(250);
	return false;
}


/**
	Pause for a given time while managing the connection.
	Returns on timeout or call clearing.
	Used for debugging to simulate ringing at terminating end.
	@param transaction The transaction record for the call.
	@param TCH The TCH+FACCH sed for this call.
	@param waitTime_ms The maximum time to wait, in ms.
	@return true If the call is cleared during the wait.
*/
bool waitInCall(TransactionEntry& transaction, TCHFACCHLogicalChannel *TCH, unsigned waitTime_ms)
{
	Timeval targetTime(waitTime_ms);
	CLDCOUT("waitInCall");
	while (!targetTime.passed()) {
		if (pollInCall(transaction,TCH)) return true;
	}
	return false;
}



/**
	This is the standard call manangement loop, regardless of the origination type.
	This function returns when the call is cleared and the channel is released.
	@param transaction The transaction record for this call.
	@param TCH The TCH+FACCH for the call.
*/
void callManagementLoop(TransactionEntry &transaction, TCHFACCHLogicalChannel* TCH)
{
	CLDCOUT("MOC MTC callManagementLoop");
	transaction.SIP().FlushRTP();
	// poll everything until the call is cleared
	CLDCOUT("callManagementLoop: entering polling loop");
	while (!pollInCall(transaction,TCH)) { }
}




/**
	This function starts MOC on the SDCCH to the point of TCH assignment. 
	@param req The CM Service Request that started all of this.
	@param LCH The logical used to initiate call setup.
*/
void Control::MOCStarter(const L3CMServiceRequest* req, LogicalChannel *LCH)
{
	assert(LCH);
	assert(req);
	CLDCOUT("MOC: " << *req);

	// Determine if very early assignment already happened.
	bool veryEarly=false;
	if (LCH->type()==FACCHType) veryEarly=true;

	// If we got a TMSI, find the IMSI.
	L3MobileIdentity mobileIdentity = req->mobileIdentity();
	if (mobileIdentity.type()==TMSIType) {
		const char *IMSI = gTMSITable.find(mobileIdentity.TMSI());
		if (IMSI) mobileIdentity = L3MobileIdentity(IMSI);
	}

	// Still no IMSI?  Ask for one.
	if (mobileIdentity.type()!=IMSIType) {
		CLDCOUT("NOTICE -- MOC with no IMSI or valid TMSI.  Reqesting IMSI.");
		LCH->send(L3IdentityRequest(IMSIType));
		// FIXME -- This request times out on T3260, 12 sec.  See GSM 04.08 Table 11.2.
		L3IdentityResponse *resp = dynamic_cast<L3IdentityResponse*>(getMessage(LCH));
		if (!resp) throw UnexpectedMessage();
		mobileIdentity = resp->mobileID();
	}

	// Still no IMSI??
	if (mobileIdentity.type()!=IMSIType) {
		// FIXME -- This is quick-and-dirty, not following GSM 04.08 5.
		CERR("WARNING -- (ControlLayer) MOC setup with no IMSI");
		// Cause 0x60 "Invalid mandatory information"
		LCH->send(L3CMServiceReject(L3RejectCause(0x60)));
		LCH->send(L3ChannelRelease());
		// The SIP side and transaction record don't exist yet.
		// So we're done.
		return;
	}

	// FIXME -- At this point, verify the that subscriber has access to this service.
	// If the subscriber isn't authorized, send a CM Service Reject with
	// cause code, 0x41, "requested service option not subscribed",
	// followed by a Channel Release with cause code 0x6f, "unspecified".
	// Otherwise, proceed to the next section of code.
	// For now, we are assuming that the phone won't make a call if it didn't
	// get registered.

	// Allocate a TCH for the call, if we don't have it already.
	TCHFACCHLogicalChannel *TCH = NULL;
	if (!veryEarly) {
		TCH = allocateTCH(dynamic_cast<SDCCHLogicalChannel*>(LCH));
		// It's OK to just return on failure; allocateTCH cleaned up already.
		if (TCH==NULL) return;
	}

	// Let the phone know we're going ahead with the transaction.
	CLDCOUT("MOC: sending CMServiceAccept")
	LCH->send(L3CMServiceAccept());

	// Get the Setup message.
	// GSM 04.08 5.2.1.2
	const L3Setup *setup = dynamic_cast<const L3Setup*>(getMessage(LCH));
	if (setup==NULL) throw UnexpectedMessage();
	CLDCOUT("MOC: " << *setup);
	// Pull out the L3 short transaction information now.
	// See GSM 04.07 11.2.3.1.3.
	unsigned L3TI = setup->TIValue();
	if (!setup->haveCalledPartyBCDNumber()) {
		// FIXME -- This is quick-and-dirty, not following GSM 04.08 5.
		CERR("WARNING -- (ControlLayer) MOC setup with no number");
		// Cause 0x60 "Invalid mandatory information"
		LCH->send(L3ReleaseComplete(0,L3TI,L3Cause(0x60)));
		LCH->send(L3ChannelRelease());
		// The SIP side and transaction record don't exist yet.
		// So we're done.
		return;
	}

	CLDCOUT("MOC: SIP start engine")
	// Get the users sip_uri by pulling out the IMSI.
	const char *IMSI = mobileIdentity.digits();
	// Pull out Number user is trying to call and use as the sip_uri.
	const char *bcd_digits = setup->calledPartyBCDNumber().digits();

	// Create a transaction table entry so the TCH controller knows what to do later.
	// The transaction on the TCH is a continuation of this one and uses the same ID.
	TransactionEntry transaction(mobileIdentity,
		req->serviceType(),
		L3TI,
		setup->calledPartyBCDNumber());
	transaction.SIP().User(IMSI);
	transaction.Q931State(TransactionEntry::MOCInitiated);
	LCH->transactionID(transaction.ID());
	if (!veryEarly) TCH->transactionID(transaction.ID());
	CLDCOUT("MOC: transaction: " << transaction);
	gTransactionTable.add(transaction);

	// At this point, we have enough information start the SIP call setup.

	// Now start a call by contacting asterisk.
	// Engine methods will return their current state.	
	// The remote party will start ringing soon.
	CLDCOUT("MOC: starting SIP (INVITE) Calling "<<bcd_digits);
	unsigned basePort = allocateRTPPorts();
	SIPState state = transaction.SIP().MOCSendINVITE(bcd_digits,"127.0.0.1",basePort,SIP::RTPGSM610);
	CLDCOUT("MOC: SIP state="<<state)
	CLDCOUT("MOC: Q.931 state=" << transaction.Q931State());

	// Finally done with the Setup message.
	delete setup;

	// The transaction is moving on to the MOCController.
	// If we need a TCH assignment, we do it here.
	gTransactionTable.update(transaction);
	CLDCOUT("MOC: transaction: " << transaction);
	if (veryEarly) {
		// For very early assignment, we need a mode change.
		static const L3ChannelMode mode(L3ChannelMode::SpeechV1);
		LCH->send(L3ChannelModeModify(LCH->channelDescription(),mode));
		const L3ChannelModeModifyAcknowledge *ack =
			dynamic_cast<L3ChannelModeModifyAcknowledge*>(getMessage(LCH));
		if (!ack) throw UnexpectedMessage();
		// Cause 0x06 is "channel unacceptable"
		if (ack->mode() != mode) return abortCall(transaction,LCH,L3Cause(0x06));
		MOCController(transaction,dynamic_cast<TCHFACCHLogicalChannel*>(LCH));
	} else {
		// For late assignment, send the TCH assignment now.
		// This dispatcher on the next channel will continue the transaction.
		assignTCHF(dynamic_cast<SDCCHLogicalChannel*>(LCH),TCH);
	}
}





/**
	Continue MOC process on the TCH.
	@param transaction The call state and SIP interface.
	@param TCH The traffic channel to be used.
*/
void Control::MOCController(TransactionEntry& transaction, TCHFACCHLogicalChannel* TCH)
{
	CLDCOUT("MOC: transaction: " << transaction);
	unsigned L3TI = transaction.TIValue();
	assert(TCH);

	// Once we can start SIP call setup, send Call Proceeding.
	CLDCOUT("MOC: Sending Call Proceeding ");
	TCH->send(L3CallProceeding(1,L3TI));
	transaction.Q931State(TransactionEntry::MOCProceeding);

	// Look for RINGING or OK from the SIP side.
	// There's a T310 running on the phone now.
	// The phone will initiate clearing if it expires.
	while (transaction.Q931State()!=TransactionEntry::CallReceived) {

		if (updateGSMSignalling(transaction,TCH)) return;
		if (transaction.clearing()) return abortCall(transaction,TCH,L3Cause(0x7F));

		CLDCOUT("MOC A: wait for Ringing or OK");
		SIPState state = transaction.SIP().MOCWaitForOK();
		CLDCOUT("MOC A: SIP state="<<state)
		switch (state) {
			case SIP::Busy:
				CLDCOUT("MOC A: SIP:Busy, abort");
				return abortCall(transaction,TCH,L3Cause(0x11));
			case SIP::Fail:
				CLDCOUT("MOC A: SIP:Fail, abort");
				return abortCall(transaction,TCH,L3Cause(0x7F));
			case SIP::Ringing:
				CLDCOUT("MOC A: SIP:Ringing, send Alerting and move on");
				TCH->send(L3Alerting(1,L3TI));
				transaction.Q931State(TransactionEntry::CallReceived);
				break;
			case SIP::Active:
				CLDCOUT("MOC A: SIP:Active, move on");
				transaction.Q931State(TransactionEntry::CallReceived);
				break;
			case SIP::Proceeding:
				CLDCOUT("MOC A: SIP:Proceeding, send progress");
				TCH->send(L3Progress(1,L3TI));
				break;
			case SIP::Timeout:
				CLDCOUT("MOC A: SIP:Timeout, reinvite");
				state = transaction.SIP().MOCResendINVITE();
				break;
			default:
				CLDCOUT("MOC A: SIP unexpected state " << state);
				break;
		}
	}

	// There's a question here of what entity is generating the "patterns"
	// (ringing, busy signal, etc.) during call set-up.  For now, we're ignoring 
	// that question and hoping the phone will make its own ringing pattern.


	// Wait for the SIP session to start.
	// There's a timer on the phone that will initiate clearing if it expires.
	CLDCOUT("MOC: wait for SIP OKAY");
	SIPState state = transaction.SIP().state();
	while (state!=SIP::Active) {

		CLDCOUT("MOC: wait for SIP session start");
		state = transaction.SIP().MOCWaitForOK();
		CLDCOUT("MOC: SIP state "<< state)

		// check GSM state
		if (updateGSMSignalling(transaction,TCH)) return;
		if (transaction.clearing()) return abortCall(transaction,TCH,L3Cause(0x7F));

		// parse out SIP state
		switch (state) {
			case SIP::Busy:
				// Should this be possible at this point?
				CLDCOUT("MOC B: SIP:Busy, abort");
				return abortCall(transaction,TCH,L3Cause(0x11));
			case SIP::Fail:
				CLDCOUT("MOC B: SIP:Fail, abort");
				return abortCall(transaction,TCH,L3Cause(0x7F));
			case SIP::Proceeding:
				CLDCOUT("MOC B: SIP:Proceeding, NOT sending progress");
				//CLDCOUT("MOC B: SIP:Proceeding, send progress");
				//TCH->send(L3Progress(1,L3TI));
				break;
			// For these cases, do nothing.
			case SIP::Timeout:
				// FIXME We should abort if this happens too often.
				// For now, we are relying on the phone, which may have bugs of its own.
			case SIP::Active:
			default:
				break;
		}
	} 
	
	// Let the phone know the call is connected.
	CLDCOUT("MOC: sending Connect to handset");
	TCH->send(L3Connect(1,L3TI));
	transaction.T313().set();
	transaction.Q931State(TransactionEntry::ConnectIndication);

	// The call is open.
	transaction.SIP().MOCInitRTP();
	transaction.SIP().MOCSendACK();

	// Get the Connect Acknowledge message.
	while (transaction.Q931State()!=TransactionEntry::Active) {
		CLDCOUT("MOC Q.931 state=" << transaction.Q931State());
		if (updateGSMSignalling(transaction,TCH,T313ms)) return abortCall(transaction,TCH,L3Cause(0x7F));
	}

	// At this point, everything is ready to run the call.
	callManagementLoop(transaction,TCH);

	// The radio link should have been cleared with the call.
	// So just return.
}




void Control::MTCStarter(TransactionEntry& transaction, LogicalChannel *LCH)
{
	assert(LCH);
	CLDCOUT("MTC on " << LCH->type() << " transaction: "<< transaction);

	// Determine if very early assigment already happened.
	bool veryEarly = false;
	if (LCH->type()==FACCHType) veryEarly=true;

	// Allocate a TCH for the call.
	TCHFACCHLogicalChannel *TCH = NULL;
	if (!veryEarly) {
		TCH = allocateTCH(dynamic_cast<SDCCHLogicalChannel*>(LCH));
		// It's OK to just return on failure; allocateTCH cleaned up already.
		// The orphaned transaction will be cleared at the next findByMobileID call.
		if (TCH==NULL) return;
	}


	// Get transaction identifiers.
	if (!veryEarly) TCH->transactionID(transaction.ID());	
	LCH->transactionID(transaction.ID());	
	unsigned L3TI = transaction.TIValue();

	// GSM 04.08 5.2.2.1
	CLDCOUT("MTC: sending GSM Setup");
	// FIXME -- Insert Calling Party BCD Number, see bug #125.
	LCH->send(L3Setup(0,L3TI,L3CallingPartyBCDNumber(transaction.calling())));
	transaction.T303().set();
	transaction.Q931State(TransactionEntry::CallPresent);

	// Wait for Call Confirmed message.
	CLDCOUT("MTC: wait for GSM Call Confirmed")
	while (transaction.Q931State()!=TransactionEntry::MTCConfirmed) {
		if (transaction.SIP().MTCSendTrying()==SIP::Fail) {
			LCH->send(RELEASE);
			// Cause 0x03 is "no route to destination"
			return abortCall(transaction,LCH,L3Cause(0x03));
		}
		// FIXME -- What's the proper timeout here?
		// It's the SIP TRYING timeout, whatever that is.
		if (updateGSMSignalling(transaction,LCH,1000)) {
			CLDCOUT("MTC: Release from GSM side");
			LCH->send(RELEASE);
			return;
		}
		// Check for SIP cancel, too.
		if (transaction.SIP().MTCWaitForACK()==SIP::Fail) {
			LCH->send(RELEASE);
			// Cause 0x10 is "normal clearing"
			return abortCall(transaction,LCH,L3Cause(0x10));
		}
	}

	// The transaction is moving to the MTCController.
	gTransactionTable.update(transaction);
	CLDCOUT("MTC: transaction: " << transaction);
	if (veryEarly) {
		// For very early assignment, we need a mode change.
		static const L3ChannelMode mode(L3ChannelMode::SpeechV1);
		LCH->send(L3ChannelModeModify(LCH->channelDescription(),mode));
		const L3ChannelModeModifyAcknowledge *ack =
			dynamic_cast<L3ChannelModeModifyAcknowledge*>(getMessage(LCH));
		if (!ack) throw UnexpectedMessage();
		// Cause 0x06 is "channel unacceptable"
		if (ack->mode() != mode) return abortCall(transaction,LCH,L3Cause(0x06));
		MTCController(transaction,dynamic_cast<TCHFACCHLogicalChannel*>(LCH));
	}
	else {
		// For late assignment, send the TCH assignment now.
		// This dispatcher on the next channel will continue the transaction.
		assignTCHF(dynamic_cast<SDCCHLogicalChannel*>(LCH),TCH);
	}
}


void Control::MTCController(TransactionEntry& transaction, TCHFACCHLogicalChannel* TCH)
{
	// Early Assignment Mobile Terminated Call. 
	// Transaction table in 04.08 7.3.3 figure 7.10a

	CLDCOUT("MTC: transaction: " << transaction);
	unsigned L3TI = transaction.TIValue();
	assert(TCH);

	// Get the alerting message.
	CLDCOUT("MTC:: waiting for GSM Alerting and Connect");
	while (transaction.Q931State()!=TransactionEntry::Active) {
		if (updateGSMSignalling(transaction,TCH,1000)) return;
		if (transaction.Q931State()==TransactionEntry::CallReceived) {
			CLDCOUT("MTC:: sending SIP Ringing");
			transaction.SIP().MTCSendRinging();
		}
		// Check for SIP cancel, too.
		if (transaction.SIP().MTCWaitForACK()==SIP::Fail) {
			return abortCall(transaction,TCH,L3Cause(0x7F));
		}
	}

	CLDCOUT("MTC:: allocating port and sending SIP OKAY");
	unsigned RTPPorts = allocateRTPPorts();
	SIPState state = transaction.SIP().MTCSendOK(RTPPorts,SIP::RTPGSM610);
	while (state!=SIP::Active) {
		CLDCOUT("MTC: wait for SIP OKAY-ACK");
		if (updateGSMSignalling(transaction,TCH)) return;
		state = transaction.SIP().MTCWaitForACK();
		CLDCOUT("MTC: SIP call state "<< state);
		switch (state) {
			case SIP::Active:
				break;
			case SIP::Fail:
				return abortCall(transaction,TCH,L3Cause(0x7F));
			case SIP::Timeout:
				state = transaction.SIP().MTCSendOK(RTPPorts,SIP::RTPGSM610);
				break;
			case SIP::Connecting:
				break;
			default:
				CLDCOUT("MTC: SIP unexpected state " << state);
				break;
		}
	}
	transaction.SIP().MTCInitRTP();

	// Send Connect Ack to make it all official.
	CLDCOUT("MTC:: send GSM Connect Acknowledge")
	TCH->send(L3ConnectAcknowledge(0,L3TI));

	// At this point, everything is ready to run for the call.
	// The radio link should have been cleared with the call.
	callManagementLoop(transaction,TCH);
}




// vim: ts=4 sw=4
