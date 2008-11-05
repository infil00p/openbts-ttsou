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



#ifndef SMS_PROCESSORS_H
#define SMS_PROCESSORS_H

#include "GSMCommon.h"
#include "GSMLogicalChannel.h"
#include "SMSTransfer.h"
#include "SMSMessages.h"


namespace SMS { 

class RLProcessor;



/**
	Same as function in Control, but extra sapi 
*/
bool waitForPrimitive(GSM::SDCCHLogicalChannel * SDCCH, 
	GSM::GSMPrimitive prim, unsigned timeout_ms=BIGREADTIMEOUT, unsigned wSAPI=3);

/**
	Same as function in Control, but extra sapi 
*/
GSM::L3Message* getMessage(GSM::SDCCHLogicalChannel * SDCCH, 
	unsigned timeout_ms=5000, unsigned wSAPI=3);






/** Defines processor for handling Connection Management (CM) sublayer
using (SM-CP protocol) ref: GSM  04.11 2.1 */  
class CMProcessor 
{
	public:
	/** All possible states depicted in Annex B SDL-19 -> SDL-24 */
	enum CMState {
		MO_Idle=0,
		MT_Idle=1,
		MO_WaitForRPAck=2,
		MO_WaitForCPAck=3,
		MT_WaitForCPAck=4,
		MT_WaitForCPData=5,
		MT_WaitForRPAck=6,
	};

	volatile bool mActive;	///< true if opened and in use.
	volatile bool mRunning; ///< true if upstream loop is processing.

	Mutex mLock;		///< protect shared variables in uplink/downlink operations.
	GSM::SDCCHLogicalChannel * mDownstream;	///< Downstream SDDCHLogicalChannel sapi=3. 
	RLProcessor * mUpstream;		///< Uplink RP-Layer.   

	CMState mState;			///< state for cp layer.
	GSM::Z100Timer mTCM1;		///< timer value in 04.11-10 is 40 seconds. 
	
	void upstreamServiceLoop();	///< link side of CP layer. 
	unsigned reTx;			///< current retransmission count.

	Thread 	mUpstreamThread;	///< Thread to receive from L3.



	void receiveL3( const CPMessage& );
	
	void receiveCPData( const CPMessage& );	
	void receiveCPAck();
	void receiveCPError( const CPMessage& );
		
	void sendCPData( const CPMessage& );
	void sendCPAck();
	void sendCPError( const CPMessage& );

	/** Drive uplink side of L2 Transceiver */

	public:

	CMProcessor():
		mDownstream(NULL), mUpstream(NULL), mTCM1(30)
	{ }

	~CMProcessor() { stop(); }


	void writeLowSide( const CPMessage& );
	void writeHighSide( const RLFrame& rpframe ); 

	void writeRLFrame( const RLFrame& outframe );

	void downstream( GSM::SDCCHLogicalChannel * wDownstream ){ mDownstream = wDownstream; }
	void upstream( RLProcessor* wUpstream){ mUpstream = wUpstream; }

	void open();
	void close();
	void stop(){}
	bool active(){ return mActive; }
	void checkTCM1();

	public:
	friend void* CMProcessorUpstreamServiceLoopAdapter(CMProcessor*);

};


void* CMProcessorUpstreamServiceLoopAdapter(CMProcessor*);

std::ostream& operator<<(std::ostream& os, CMProcessor::CMState);


class RLProcessor 
{

//	enum RPState {
//		Idle=0,		
//		WaitForRPAck=1,
//		WaitToSendRPAck=3
//	};

//	void receiveRLFrame( const RLFrame& );

//	void receiveRPData( const RLFrame& );	
//	void receiveRPAck( const RLFrame& );
//	void receiveRPSMMA( const RLFrame& );
//	void receiveRPError( const RLFrame& );

//	void sendRPData( const RLFrame& );
//	void sendRPAck( const RLFrame& );
//	void sendRPSMMA( const RLFrame& );
//	void sendRPError( const RLFrame& );

	public:
	RLProcessor() {}

	void writeLowSide( const RLFrame& rpframe );
//	void writeHighSide( const TLFrame& rpframe ){}

//	void downstream( CPProcessor* wDownstream) { mDownstream=wDownstream; }
//	void upstream( RPProcessor* wUpstream) { mUpstream = wUpstream; }

//	friend void * RPProcessorUpstreamServiceLoopAdapter(RPProcessor*);

};

//void * RPProcessorUpstreamServiceLoopAdapter(RPProcessor*);
//std::ostream& operator<<(std::ostream& os, CPProcessor::CPState);





}; //namespace SMS
#endif
