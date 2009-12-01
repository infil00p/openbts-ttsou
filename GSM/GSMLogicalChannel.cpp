/**@file Logical Channel.  */

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



#include "GSML3RRElements.h"
#include "GSML3Message.h"
#include "GSML3RRMessages.h"
#include "GSMLogicalChannel.h"
#include "GSMConfig.h"

using namespace std;
using namespace GSM;



void LogicalChannel::open()
{
	LOG(INFO);
	if (mSACCH) mSACCH->open();
	if (mL1) mL1->open();
	for (int s=0; s<4; s++) {
		if (mL2[s]) mL2[s]->open();
	}
}


void LogicalChannel::connect()
{
	mMux.downstream(mL1);
	if (mL1) mL1->upstream(&mMux);
	for (int s=0; s<4; s++) {
		mMux.upstream(mL2[s],s);
		if (mL2[s]) mL2[s]->downstream(&mMux);
	}
}


void LogicalChannel::downstream(ARFCNManager* radio)
{
	assert(mL1);
	mL1->downstream(radio);
	if (mSACCH) mSACCH->downstream(radio);
}



// Serialize and send an L3Message with a given primitive.
void LogicalChannel::send(const L3Message& msg,
		const GSM::Primitive& prim,
		unsigned SAPI)
{
	LOG(INFO) << "L3 SAP" << SAPI << " sending " << msg;
	send(L3Frame(msg,prim), SAPI);
}




CCCHLogicalChannel::CCCHLogicalChannel(const TDMAMapping& wMapping)
	:mRunning(false)
{
	mL1 = new CCCHL1FEC(wMapping);
	mL2[0] = new CCCHL2;
	connect();
}


void CCCHLogicalChannel::open()
{
	LogicalChannel::open();
	if (!mRunning) {
		mRunning=true;
		mServiceThread.start((void*(*)(void*))CCCHLogicalChannelServiceLoopAdapter,this);
	}
}


void CCCHLogicalChannel::serviceLoop() 
{
	// build the idle frame
	static const L3PagingRequestType1 filler;
	static const L3Frame idleFrame(filler,UNIT_DATA);
	// prime the first idle frame
	LogicalChannel::send(idleFrame);
	// run the loop
	while (mRunning) {
		L3Frame* frame = mQ.read();
		if (frame) {
			LogicalChannel::send(*frame);
			OBJLOG(DEBUG) << "CCCHLogicalChannel::serviceLoop sending " << *frame;
			delete frame;
		}
		if (mQ.size()==0) {
			LogicalChannel::send(idleFrame);
			OBJLOG(DEEPDEBUG) << "CCCHLogicalChannel::serviceLoop sending idle frame";
		}
	}
}


void *GSM::CCCHLogicalChannelServiceLoopAdapter(CCCHLogicalChannel* chan)
{
	chan->serviceLoop();
	return NULL;
}



L3ChannelDescription LogicalChannel::channelDescription() const
{
	// In some debug cases, L1 may not exist, so we fake this information.
	if (mL1==NULL) return L3ChannelDescription(TDMA_MISC,0,0,0);

	// In normal cases, we get this information from L1.
	return L3ChannelDescription(
		mL1->typeAndOffset(),
		mL1->TN(),
		mL1->TSC(),
		mL1->ARFCN()
	);
}




SDCCHLogicalChannel::SDCCHLogicalChannel(
		unsigned wTN,
		const CompleteMapping& wMapping)
{
	mL1 = new SDCCHL1FEC(wTN,wMapping.LCH());
	// SAP0 is RR/MM/CC, SAP3 is SMS
	// SAP1 and SAP2 are not used.
	L2LAPDm *SAP0L2 = new SDCCHL2(1,0);
	L2LAPDm *SAP3L2 = new SDCCHL2(1,3);
	LOG(DEBUG) << "LAPDm pairs SAP0=" << SAP0L2 << " SAP3=" << SAP3L2;
	SAP3L2->master(SAP0L2);
	mL2[0] = SAP0L2;
	mL2[3] = SAP3L2;
	mSACCH = new SACCHLogicalChannel(wTN,wMapping.SACCH());
	connect();
}





SACCHLogicalChannel::SACCHLogicalChannel(
		unsigned wTN,
		const MappingPair& wMapping)
		: mRunning(false)
{
	mL1 = new SACCHL1FEC(wTN,wMapping);
	// SAP0 is RR, SAP3 is SMS
	// SAP1 and SAP2 are not used.
	mL2[0] = new SACCHL2(1,0);
	mL2[3] = new SACCHL2(1,3);
	connect();
	assert(mSACCH==NULL);
}


void SACCHLogicalChannel::open()
{
	LogicalChannel::open();
	if (!mRunning) {
		mRunning=true;
		mServiceThread.start((void*(*)(void*))SACCHLogicalChannelServiceLoopAdapter,this);
	}
}



void SACCHLogicalChannel::serviceLoop() 
{
	// run the loop
	// For now, just send SI5 and SI6 over and over.
	// Later, we can add an incoming FIFO from L2.
	while (mRunning) {
		if (active()) {
			OBJLOG(DEEPDEBUG) << "SACCHLogicalChannel::serviceLoop() sending SI5";
			LogicalChannel::send(gBTS.SI5Frame());
			OBJLOG(DEEPDEBUG) << "SACCHLogicalChannel::serviceLoop() sending SI6";
			LogicalChannel::send(gBTS.SI6Frame());
		} else {
			sleepFrames(51);
		}
	}
}


void *GSM::SACCHLogicalChannelServiceLoopAdapter(SACCHLogicalChannel* chan)
{
	chan->serviceLoop();
	return NULL;
}



TCHFACCHLogicalChannel::TCHFACCHLogicalChannel(
		unsigned wTN,
		const CompleteMapping& wMapping)
{
	mTCHL1 = new TCHFACCHL1FEC(wTN,wMapping.LCH());
	mL1 = mTCHL1;
	// SAP0 is RR/MM/CC, SAP3 is SMS
	// SAP1 and SAP2 are not used.
	mL2[0] = new FACCHL2(1,0);
	mL2[3] = new FACCHL2(1,3);
	mSACCH = new SACCHLogicalChannel(wTN,wMapping.SACCH());
	connect();
}





// vim: ts=4 sw=4

