/**@file
	@brief Connection Managment Layer (L3) for SMS. 
	referencing GSM 04.11 
	NOTE -- all fsm details taken from Appendix B: SDL-19 to SDL-24 
*/

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
Raffi Sevlian, raffisev@gmail.com
*/



#include <stdio.h>
#include "Common.h"
#include "ControlCommon.h"
#include "SMSTransfer.h"
#include "SMSProcessors.h"


using namespace std;
using namespace GSM;
using namespace SMS;
using namespace Control;

#define DEBUG 1


bool SMS::waitForPrimitive(SDCCHLogicalChannel * SDCCH, GSMPrimitive prim, 
	unsigned timeout_ms, unsigned wSAPI )
{
	bool waiting = true;
	while (waiting) {
		ABisTransfer *req = SDCCH->recvL3(timeout_ms, wSAPI);
		if (req==NULL) {
			CERR("NOTICE -- (ControlLayer) waitForPrimitive timed out");
			return false;
		}
		waiting = (req->primitive()!=prim);
		delete req;
	}
	return true;
}


L3Message* SMS::getMessage(SDCCHLogicalChannel *SDCCH, unsigned timeout_ms, unsigned wSAPI)
{
	ABisTransfer *rcv = SDCCH->recvL3(timeout_ms, wSAPI);
	if (rcv==NULL) {
		CERR("NOTICE -- getMessage timed out");
		throw ChannelReadTimeout();
	}
	//CLDCOUT("getMessage got " << *rcv);
	GSMPrimitive primitive = rcv->primitive();
	L3Message *msg = rcv->takeMessage();
	delete rcv;
	if ((primitive!=DATA_INDICATION) && (primitive!=UNIT_DATA_INDICATION)) {
		CERR("NOTICE -- getMessage got unexpected primitive " << primitive);
		throw UnexpectedPrimitive();
	}
	if (msg==NULL) {
		CERR("NOTICE -- getMessage got unparsed message");
		throw UnsupportedMessage();
	}
	return msg;
}


ostream& SMS::operator<<(ostream& os, CMProcessor::CMState state)
{
	switch(state){
		case CMProcessor::MT_Idle: os << "MT-Idle "; break;
		case CMProcessor::MT_WaitForCPAck: os << "MT-WaitForCPAck "; break;
		case CMProcessor::MT_WaitForCPData: os << "MT-WaitForCPData "; break;
		default : os <<" ? "<<(int)state<<"?";	
	}
	return os;
}


void CMProcessor::writeRLFrame( const RLFrame& outframe )
{
	DCOUT("CMProcessor::writeRLFrame: ")
	if (mUpstream==NULL) {
		CERR("Warning -- L2 not connected to L3, dropped L3Frame ")
		return;
	}
	mUpstream->writeLowSide(outframe);
}


void CMProcessor::receiveL3( const CPMessage& msg )
{
	switch( msg.MTI() ) {
		case CPMessage::Data: receiveCPData(msg); break;
		case CPMessage::Ack: receiveCPAck(); break;
		case CPMessage::Error: receiveCPError(msg); break;

		// FIXME -- make a proper exception class 
		default: SMS_ERROR; 
	}
}

// ref. GSM 04.11 Annex B SDL-19 & SDL-24 
void CMProcessor::receiveCPData( const CPMessage& msg )
{
	// Make sure ack is occuring while in correct state 
	if( mState != MT_WaitForCPData ||  mState != MO_Idle ){
		CERR("Warning -- CMProcessor::receiveCPData: CP-Data arrived in state="<<mState)
		SMS_ERROR;	
	}

	// update state 
	switch(mState){
		// ref. SDL-19 MO-Idle 
		case MO_Idle: 
			mState=MO_WaitForRPAck; 
			//writeRLFrame( RLFrame(cpframe.payloadAlias()), MNSMS_EST_IND );	
			break;
		
		// ref. SDL-24 
		case MT_WaitForCPData: 
			mState=MT_Idle; 
			//writeRLFrame( RLFrame(cpframe.payloadAlias()), MNSMS_DATA_IND );	
			break;
		
		// Should never happen. 
		default :
			SMS_ERROR; 
	}
	

	// send cp-ack 
	sendCPAck();
}



// ref. GSM 04.11 Annex B SDL-21 & SDL-23 for processing details 
void CMProcessor::receiveCPAck()
{
	// make sure ack is occuring while if correct state 
	if( mState != MT_WaitForCPAck || mState != MO_WaitForCPAck ){
		CERR("Warning -- CMProcessor::receiveAck: CP-Ack arrived in state="<<mState)
		SMS_ERROR;	
	}
}

void CMProcessor::receiveCPError( const CPMessage& msg )
{ }

void CMProcessor::writeHighSide( const RLFrame& frame )
{
	DCOUT("CMProcessor::writeHighSide: "<<frame<<"state="<<mState)
	if(!active()){ 
		CERR("WARNING -- ")
		return;
	}

	switch(frame.primitive()){
		// ref. SDL-20 
		case MNSMS_DATA_REQ: 
			//sendCPData(outframe); 
			break;

		// ref. SDL-22 
		case MNSMS_EST_REQ: 
			//sendCPData(outframe); 
			break;
	
		// ref. SDL-24 & SDL-20 
		case MNSMS_ABORT_REQ: 
			//sendCPError(outframe); 
			break;
		default :
			CERR(" WARNING -- undefined primitive "<<frame.primitive() )
	}
}

// ref. SDL-20, SDL-22 
void CMProcessor::sendCPData( const CPMessage& msg )
{
	// make sure ack is occuring while if correct state 
	if( mState != MT_WaitForRPAck || mState != MO_WaitForCPAck ){
		CERR("Warning -- CMProcessor::sendCPData: CP-Ack arrived in state="<<mState)
		SMS_ERROR;	
	}
	
	//  update state 
	switch(mState)
	{
		// ref. SDL-22 
		case MT_Idle:
			mState=MT_WaitForCPAck;
			break;
		// ref. SDL-20 
		case MT_WaitForRPAck:
			mState=MT_WaitForCPAck;
			break;

		default: SMS_ERROR;
	}
	
	// write message to SDCCH 
	//writeL3Frame(cpframe);
	// reset timeout timer 
	mTCM1.set();

}

void CMProcessor::sendCPAck()
{
//	CPAck ack;
//	writeCMFrame(ack); 
}

// ref. SDL-20 & SDL-24 
void CMProcessor::sendCPError( const CPMessage& msg )
{
	// make sure ack is occuring while if correct state 
	if( mState != MT_WaitForCPData || mState != MO_WaitForRPAck ){
		CERR("Warning -- CMProcessor::receiveAck: CP-Ack arrived in state="<<mState)
		SMS_ERROR;	
	}

	// update state 
	switch(mState)
	{
		// ref. SDL-20 
		case MT_WaitForRPAck:
			mState=MT_Idle;
			break;

		// ref. SDL-24 
		case MT_WaitForCPData:
			mState=MT_Idle;
			break;

		default: SMS_ERROR;
	}

	//writeL3Frame( CMFrame(CMHeader::CPError));
}


// upstream loop 
void CMProcessor::upstreamServiceLoop()
{
	while(mRunning) {
		
		// read the Logical Channel for SMS. SAPI=3 
		//AbisTransfer * abis		

		//  process timeouts 
		//if( msg == NULL ){
		//	checkTC1M();
		//	continue;
		//}
	
		// Normal Operation receive CM Frame and
		//process FSM. 
		if(mActive) {
			//receiveL3(cmframe);
		} else {
			DCOUT("non-active CMProcessor receive: ");//<< *cmframe )
		}
	}
}

void CMProcessor::checkTCM1()
{
	DCOUT(" CMProcessor::checkTCM1 reTx="<< reTx )
	
	if (!mActive) {
		DCOUT(" CMProcessor::checkTCM1 -- non-active CMProcessor ")
		return;
	} 
	if (!mTCM1.expired()) {
		DCOUT(" CMProcessor::checkTCM1 -- timer not expired ")
		return;
	}

	DCOUT(" CMProcessor::checkTCM1 -- timer expired ")
	reTx++;
	if (reTx>10) {
		//abnormalRelease();
		return;
	}
}

void * SMS::CMProcessorUpstreamServiceLoopAdapter( CMProcessor* proc )
{
	proc->upstreamServiceLoop();
	return NULL;
}
