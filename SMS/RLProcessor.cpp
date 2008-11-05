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
#include "GSMCommon.h"
#include "GSMLogicalChannel.h"
#include "ControlCommon.h"

#include "SMSTransfer.h"
#include "CMMessage.h"
#include "RLMessage.h"

#include "CMProcessor.h"
#include "TLProcessor.h"
#include "RLProcessor.h"


using namespace std;
using namespace SMS;


ostream& SMS::operator<<(ostream& os, RLProcessor::RLState state)
{
	switch(state) {
		case RLProcessor::Idle: os <<"Idle "; break;
		case RLProcessor::MT_WaitForRPAck: os << "MT_WaitForRPAck "; break;
		case RLProcessor::MO_WaitToSendRPAck: os << "MO_WaitToSendRPData"; break;
		default : os<<" ? "<<(int)state<<"?";
	}
	return os;
}


void RLProcessor::open()
{
	DCOUT("RLProcessor::open ")
	if(mRunning) start();
	mActive=true;
}

void RLProcessor::close()
{
	DCOUT("RLProcessor::close")
	mActive=false;
}
	
void RLProcessor::start()
{
	DCOUT(" RLProcessor::start")
	mRunning=true;
}

void RLProcessor::stop()
{
	DCOUT("RLProcessor::stop")
	close();
	mRunning=false;
}

void RLProcessor::writeRPMessage( const RPMessage& msg, const SMSPrimitive& prim )
{
	DCOUT("RLProcessor::writeRPMessage ")
	if (mDownstream==NULL) {
		CERR("WARNING -- no uplink RLProcessor::writeTLFrame")
		return;
	}

	RLFrame frame(msg.bitsNeeded(), prim);	
	msg.write(frame);
	DCOUT("RLProcessor::writeRPMessage frame="<<frame)
	mDownstream->writeHighSide(frame);
}


void RLProcessor::writeLowSide( const RLFrame& frame ) 
{
	DCOUT("RLProcessor::writeLowSide state="<<mState <<" frame="<<frame)

	// Mobile Originated SDL-5
	if ( mState == Idle && frame.primitive() == MNSMS_EST_IND ) {	
		RPMessage * msg = parseRP(frame);
		writeTLFrame( TLFrame( msg->payload(), SM_RL_DATA_IND)  );
		mState = MO_WaitToSendRPAck;
		DCOUT("RLProcessor::writeLowSide state="<<mState)
	} else { SMS_ERROR; }				

}

void RLProcessor::writeHighSide( const TLFrame& frame )
{
		
	DCOUT("RLProcessor::writeHighSide state="<<mState <<" frame="<<frame)
	if( mState == MO_WaitToSendRPAck && frame.primitive() == SM_RL_REPORT_REQ ){

		RPAck ack(frame);
		writeRPMessage((RPMessage&)ack, MNSMS_DATA_REQ);

		//mDownstream->writeHighSide(RLFrame(MNSMS_REL_REQ));
		mState=Idle;	
	}
	else { SMS_ERROR; }
}




void RLProcessor::writeTLFrame( const TLFrame& frame )
{
	DCOUT("RLProcessor::writeTLFrame frame="<<frame)
	if(mUpstream==NULL){
		CERR("WARNING -- no uplink TLProcessor::writeTLFrame")
		return ;
	}
	mUpstream->writeLowSide( frame );
}






