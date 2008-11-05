/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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


#ifndef CM_PROCESSOR_H
#define CM_PROCESSOR_H

namespace SMS { 

class RLProcessor;






/** Defines processor for handling Connection Management (CM) sublayer
using (SM-CP protocol) ref: GSM  04.11 2.1 */  
class CMProcessor 
{
	public:

	// All possible states depicted in Annex B SDL-19 -> SDL-24 
	enum CMState {
		MO_Idle=0,
		MO_WaitForRPAck=2,
		MO_WaitForCPAck=4,

		MT_Idle=1,
		MT_WaitForCPAck=3,
		MT_WaitForCPData=5
	};

	volatile bool mActive;	///< true if opened and in use.
	volatile bool mRunning; ///< true if upstream loop is processing.
	volatile bool mMobileOriginated;

	Mutex mLock;		///< protect shared variables in uplink/downlink operations.
	GSM::SDCCHLogicalChannel * mDownstream;	///< Downstream SDDCHLogicalChannel sapi=3. 

	RLProcessor * mUpstream;	///< Uplink RP-Layer.   
	CMState mState;			///< state for cp layer.
	GSM::Z100Timer mTCM1;	///< timer value in 04.11-10 is 40 seconds. 
	unsigned mTIFlag;
	unsigned mTIValue;

	
	void upstreamServiceLoop();	///< link side of CP layer. 
	unsigned reTx;			///< current retransmission count.
	unsigned mMaxReTx;		///< max recount value
	Thread 	mUpstreamThread;	///< Thread to receive from L3.

	/* 
		if timeout increment retx- return true, if timout and 
		max retx send uplink error -return false. 
	*/
	bool checkTC1N();
	void resetTC1N();

	void receiveL3( const CPMessage& );
	
	void receiveCPData( const CPMessage& );	
	void receiveCPAck();
	void receiveCPError( const CPMessage& );
		
	void sendCPData( const RLFrame&  );
	void sendCPAck();
	void sendCPError();


	CMProcessor():
		mDownstream(NULL), mUpstream(NULL), mTCM1(30)
	{ }

	~CMProcessor() { stop(); }

	// all CMMessages need the same TI:flag+value so
	// this is set in the control layer or someplace else
	// after receiving first message.
	void TIFlag( unsigned wTIFlag ){ mTIFlag = wTIFlag; };
	void TIValue( unsigned wTIValue ) { mTIValue = wTIValue; };


	void writeLowSide( const CPMessage& );
	void writeHighSide( const RLFrame& rpframe ); 

	void writeRLFrame( const RLFrame& outframe );
	void writeCPMessage( const CPMessage& msg );

	void downstream( GSM::SDCCHLogicalChannel * wDownstream ){ mDownstream = wDownstream; }
	void upstream( RLProcessor* wUpstream){ mUpstream = wUpstream; }

	void open( bool wMobileOriginated=false);
	void close();
	void stop();
	void start();
	bool active(){ return mActive; }
	void checkTCM1();

	public:
	friend void* CMProcessorUpstreamServiceLoopAdapter(CMProcessor*);

};


void* CMProcessorUpstreamServiceLoopAdapter(CMProcessor*);

std::ostream& operator<<(std::ostream& os, CMProcessor::CMState);


}; //namespace SMS
#endif
