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



#include <stdio.h>
#include "Common.h"

#include "GSMCommon.h"
#include "GSMLogicalChannel.h"
#include "ControlCommon.h"

#include "SMSTransfer.h"
#include "CMMessage.h"

#include "SMSTransfer.h"

#include "CMProcessor.h"
#include "RLProcessor.h"



using namespace std;
using namespace GSM;
using namespace SMS;
using namespace Control;

#define DEBUG 1



ostream& SMS::operator<<(ostream& os, CMProcessor::CMState state)
{
	switch(state) {
		case CMProcessor::MO_Idle: os << "MO-Idle "; break;
		case CMProcessor::MO_WaitForRPAck: os << "MO-WaitForRPAck "; break;
		case CMProcessor::MO_WaitForCPAck: os << "MO-WaitForCPAck "; break;

		case CMProcessor::MT_Idle: os << "MT-Idle "; break;
		case CMProcessor::MT_WaitForCPAck: os << "MT-WaitForCPAck "; break;
		case CMProcessor::MT_WaitForCPData: os << "MT-WaitForCPData "; break;

		default : os <<" ? "<<(int)state<<"?";	
	}
	return os;
}

void CMProcessor::open( bool wMobileOriginated )
{
	DCOUT("CMProcessor::open ")
	if(!mRunning) start();
	mActive=true;
	mMobileOriginated=wMobileOriginated;	
}

void CMProcessor::close()
{
	DCOUT("CMProcessor::close ")
	mActive=false;
}

void CMProcessor::start()
{
	DCOUT("CMProcessor::start ")
	mUpstreamThread.start((void*(*)(void*))CMProcessorUpstreamServiceLoopAdapter,(void*)this);
	mRunning=true;	
}

void CMProcessor::stop() 
{
	DCOUT("CMProcessor::stop ")
	close();
	mRunning=false;
}

void CMProcessor::writeRLFrame( const RLFrame& outframe )
{
	DCOUT("CMProcessor::writeRLFrame: "<<outframe)
	if (mUpstream==NULL) {
		CERR("Warning -- RLProc not connected to CMProc, dropped RLFrame ")
		return;
	}
	mUpstream->writeLowSide(outframe);
}

void CMProcessor::writeCPMessage( const CPMessage& msg )
{
	DCOUT("CMProcessor::writeCPMessage "<<msg)
	if(mDownstream==NULL) {
		CERR("Warning -- L2 not connected to L3, dropped L3Frame ")
		return;
	}
	mDownstream->sendL3(msg,DATA_REQUEST, 3);	
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

// SDL-20
void CMProcessor::sendCPData( const RLFrame& frame )
{
	DCOUT("CMProcessor::sendCPData state="<<mState) 
	// SDL-20
	if( mState==MO_WaitForRPAck ) {
		CPData data(frame.segmentAlias(0));
		data.TIFlag(mTIFlag);
		data.TIValue(mTIValue);
		writeCPMessage(data);
		mState = MO_WaitForCPAck;
		DCOUT("CMProcessor::sendCPData new_state="<<mState) 
	}

	// SDL-22
	else if( mState == MT_Idle ) {
		CPData data(frame.segmentAlias(0));
		writeCPMessage(data);
		mState = MT_WaitForCPAck;
		DCOUT("CMProcessor::sendCPData new_state="<<mState) 
	}

	else {SMS_ERROR;}
}

// SDL-19 
void CMProcessor::sendCPAck()
{
	DCOUT("CMProcessor::sendCPAck state="<<mState) 
	// SDL-19 
	if( mState == MO_Idle ){
		writeCPMessage(CPAck(mTIFlag, mTIValue));
	}

	else { SMS_ERROR; }
}

void CMProcessor::sendCPError()
{	
	DCOUT("CMProcessor::sendCPError state="<<mState)
	// SDL-20
	if( mState == MO_WaitForRPAck ) {
		
		writeCPMessage(CPError(0x6D, mTIFlag, mTIValue));
		mState = MO_Idle;
		DCOUT("CMProcessor::sendCPError new_state="<<mState)
	}

	// SDL-24
	else if( mState == MT_WaitForCPData ) {
		writeCPMessage(CPError());
		mState = MT_Idle;
	}
	else { SMS_ERROR; }
}





void CMProcessor::receiveCPData( const CPMessage& msg )
{ 
	DCOUT("CMProcessor::receiveCPData state="<<mState) 

	// SDL-19
	if( mState == MO_Idle )  { 
		writeRLFrame( RLFrame(msg.payload(), MNSMS_EST_IND) );
		sendCPAck();
		mState = MO_WaitForRPAck;
		DCOUT("CMProcessor::receiveCPData new_state="<<mState) 
	}

	// SDL-24
	else if( mState == MT_WaitForCPData ) {
		writeRLFrame( RLFrame(MNSMS_DATA_IND) );
		sendCPAck();	
		mState = MT_Idle;
	}

	// unknown. throw error
	else { SMS_ERROR; }
}


// SDL-21 && SDL-23
void CMProcessor::receiveCPAck()
{
	DCOUT("CMProcessor::receiveCPAck state="<<mState) 
	// SDL-21 
	if( mState == MO_WaitForCPAck ){
		mState = MO_Idle;
		DCOUT("CMProcessor::receiveCPAck new_state="<<mState) 
	}

	// SDL-23
	else if( mState == MT_WaitForCPAck ){

	}

	else { SMS_ERROR; }	

}

void CMProcessor::receiveCPError( const CPMessage& msg )
{ }

void CMProcessor::writeHighSide( const RLFrame& frame )
{
	DCOUT("CMProcessor::writeHighSide frame="<<frame)

	if(!active()) return;
	if(mDownstream==NULL) {
		CERR(" L3 not connected to CM ")
		return;
	}

	mLock.lock();
	DCOUT("upstreamServiceLoop, get lock"); 

	switch(frame.primitive()) {
		// SDL-20, SDL-22
		case MNSMS_DATA_REQ: sendCPData(frame); break;
		case MNSMS_ABORT_REQ: sendCPError(); break;
		default :
			CERR("WARNING -- unhandled primitive "<<(int)frame.primitive());	
	}

	DCOUT("upstreamServiceLoop, unlock"); 
	mLock.unlock();
}


// upstream loop 
void CMProcessor::upstreamServiceLoop()
{
	while( mRunning ) {
		// read the Logical Channel for SMS. SAPI=3 
		ABisTransfer * abis = mDownstream->recvL3(3);	
		if( abis==NULL ){ return; }

		mLock.lock();
		DCOUT("upstreamServiceLoop, get lock"); 
		// Normal Operation receive CM Frame and process FSM. 
		if( mActive && abis->message() !=NULL ) {
			DCOUT(" CMProcessor receiveL3: "<< *abis);
			receiveL3(dynamic_cast<const CPMessage&>(*abis->message()));
		} else {
			DCOUT("non-active CMProcessor receive: ");
		}

		DCOUT("upstreamServiceLoop, unlock");	
		mLock.unlock();

	}
}

bool CMProcessor::checkTC1N()
{
	DCOUT(" CMProcessor::checkTCM1 reTx="<< reTx )
	
	if (!mTCM1.expired()) {
		DCOUT(" CMProcessor::checkTCM1 -- timer not expired ")
		return true;
	}

	DCOUT(" CMProcessor::checkTCM1 -- timer expired ")
	reTx++;
	if (reTx> mMaxReTx) {
		writeRLFrame( RLFrame( MNSMS_ERROR_IND) );	
		return false;
	}
	return true;
}

void CMProcessor::resetTC1N()
{
	mTCM1.reset();
	reTx=0;
}


void * SMS::CMProcessorUpstreamServiceLoopAdapter( CMProcessor* proc )
{
	proc->upstreamServiceLoop();
	return NULL;
}




