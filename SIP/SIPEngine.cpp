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



#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include <sys/types.h>
#include <semaphore.h>

#include "GSMConfig.h"
#include "ControlCommon.h"
#include "GSMCommon.h"

#include "SIPInterface.h"
#include "SIPUtility.h"
#include "SIPMessage.h"
#include "SIPEngine.h"


using namespace std;
using namespace SIP;
using namespace GSM;
using namespace Control;



#define SEDCOUT(x) DCOUT("SIPEngine: "<<x);



ostream& SIP::operator<<(ostream& os, SIP::SIPState s)
{
	switch(s)
	{	
		case NullState:		os<<"Null"; break;
		case Timeout :	 	os<<"Timeout"; break;
		case Starting : 	os<<"Starting"; break;
		case Proceeding : 	os<<"Proceeding"; break;
		case Ringing : 		os<<"Ringing"; break;
		case Connecting : 	os<<"Connecting"; break;
		case Active :		os<<"Active"; break;
		case Fail:			os<<"Fail"; break; 
		case Busy:			os<<"Busy"; break;
		case Clearing:		os<<"Clearing"; break;
		case Cleared:		os<<"Cleared"; break;
		default: os << "??" << (int)s << "??";
	}
	return os;
}



SIPEngine::~SIPEngine()
{
	if (mINVITE==NULL) osip_message_free(mINVITE);
	if (mOK==NULL) osip_message_free(mOK);
	if (mBYE==NULL) osip_message_free(mBYE);
}



void SIPEngine::saveINVITE(const osip_message_t *INVITE)
{
	if (mINVITE!=NULL) osip_message_free(mINVITE);
	osip_message_clone(INVITE,&mINVITE);
}

void SIPEngine::saveOK(const osip_message_t *OK)
{
	if (mOK!=NULL) osip_message_free(mOK);
	osip_message_clone(OK,&mOK);
}

void SIPEngine::saveBYE(const osip_message_t *BYE)
{
	if (mBYE!=NULL) osip_message_free(mBYE);
	osip_message_clone(BYE,&mBYE);
}



void SIPEngine::User( const char * w_username )
{
	unsigned id = random();
	char tmp[20];
	sprintf(tmp, "%u", id);
	mCallID = tmp; 
	sip_username = w_username;
}
	


void SIPEngine::User( const char * wCallID, const char * w_username ) 
{
	sip_username = w_username;
	mCallID = wCallID;
}



bool SIPEngine::Register( Method wMethod )
{
	SEDCOUT("SIPEngine::Register mState=" << mState << " " << wMethod);

	// Before start, need to add mCallID
	gSIPInterface.map().add(mCallID);

	// Initial configuration for sip message.
	// Make a new from tag and new branch.
	// make new mCSeq.
	char tmp[100];
	make_tag(tmp);
	from_tag=tmp;	
	make_branch(tmp);
	via_branch=tmp;
	mCSeq = random()%600;
	
	// Generate SIP Message 
	// Either a register or unregister. Only difference 
	// is expiration period.
	osip_message_t * reg; 
	if (wMethod == SIPRegister ){
		// FIXME -- The registration period, in seconds, should be 2*GSM::T3212ms/1000.
		reg = sip_register( sip_username.c_str(), 
			local_port, asterisk_ip.c_str(), 
			asterisk_ip.c_str(), from_tag.c_str(), 
			via_branch.c_str(), mCallID.c_str(), mCSeq
		); 		
	} else if (wMethod == SIPUnregister ) {
		reg = sip_unregister( sip_username.c_str(), 
			local_port, asterisk_ip.c_str(), 
			asterisk_ip.c_str(), from_tag.c_str(), 
			via_branch.c_str(), mCallID.c_str(), mCSeq
		);
	} else abort();
 
	// Write message and delete message to
	// prevent memory leak.	
	SEDCOUT("SIPEngine::Register writing " << reg);
	gSIPInterface.write(reg);	
	osip_message_free(reg);

	// Poll the message FIFO until timeout or OK.
	// SIPInterface::read will throw SIPTIimeout if it times out.
	// It should not return NULL.
	try {
		static const int SIPTimeout = 10000;
		osip_message_t *msg = gSIPInterface.read(mCallID, SIPTimeout);
		assert(msg);
		while (msg->status_code!=200) {
			// Looking for 200 OK.
			// But will keep waiting is we get 200 Trying.
			SEDCOUT("SIPEngine::Register received status " << msg->status_code);
			if (msg->status_code!=100) {
				SEDCOUT("SIPEngine::Register unexpected message");
				gSIPInterface.map().remove(mCallID);	
				osip_message_free(msg);
				return false;
			}
			osip_message_free(msg);
			msg = gSIPInterface.read(mCallID, SIPTimeout);
			assert(msg);
		}
		SEDCOUT("SIPEngine::Register success");
		gSIPInterface.map().remove(mCallID);	
		osip_message_free(msg);
		return true;
	}
	catch (SIPTimeout) {
		SEDCOUT("SIPEngine::Register: timed out aborting ...");
		CERR("WARNING -- SIP register timed out.  Is Asterisk OK?");
		gSIPInterface.map().remove(mCallID);	
		return false;
	}
}


SIPState SIPEngine::MOCSendINVITE( const char * wCalledUsername, 
	const char * wCalledDomain , short wRtp_port, unsigned  wCodec)
{
	SEDCOUT("SIPEngine::MOCSendINVITE mState=" << mState);
	// Before start, need to add mCallID
	gSIPInterface.map().add(mCallID);
	
	// Set Invite params. 
	// New from tag + via branch
	// new CSEQ and codec 
	char tmp[100];
	make_tag(tmp);
	from_tag = tmp;
	make_branch(tmp);
	via_branch = tmp;
	codec = wCodec;
	mCSeq++;

	mCalledUsername = wCalledUsername;
	mCalledDomain = wCalledDomain;
	rtp_port= wRtp_port;

	osip_message_t * invite = sip_invite(
		mCalledUsername.c_str(), rtp_port, sip_username.c_str(), 
		local_port, asterisk_ip.c_str(), asterisk_ip.c_str(), 
		from_tag.c_str(), via_branch.c_str(), mCallID.c_str(), mCSeq, codec); 
	
	// Send Invite to Asterisk.
	gSIPInterface.write(invite);
	saveINVITE(invite);
	osip_message_free(invite);
	mState = Starting;
	return mState;
};


SIPState SIPEngine::MOCResendINVITE()
{
	assert(mINVITE);
	gSIPInterface.write(mINVITE);
	return mState;
}

SIPState  SIPEngine::MOCWaitForOK()
{
	SEDCOUT("SIPEngine::MOCWaitForOK mState=" << mState)

	osip_message_t * msg;

	// Read off the fifo. if time out will
	// clean up and return false.
	try {
		msg = gSIPInterface.read(mCallID, INVITETimeout);
	}
	catch (SIPTimeout& e) { 
		SEDCOUT("SIPEngine::MOCWaitForOK: TIMEOUT") 
		mState = Timeout;
		return mState;
	}

	SEDCOUT("SIPEngine::MOCWaitForOK: received "<<msg->status_code ) 
	switch (msg->status_code) {
		case 100:
		case 183:
			SEDCOUT("SIPEngine::MOCWaitForOK: TRYING/PROGRESS");
			mState = Proceeding;
			break;
		case 180:
			SEDCOUT("SIPEngine::MOCWaitForOK: RINGING");
			mState = Ringing;
			break;
		case 200:
			SEDCOUT("SIPEngine::MOCWaitForOK: OKAY");
			// save the OK message for the eventual ACK
			saveOK(msg);
			mState = Active;
			break;
		case 486:
			SEDCOUT("SIPEngine::MOCWaitForOK: BUSY");
			mState = Busy;
			break;
		default:
			SEDCOUT("SIPEngine::MOCWaitForOK: unhandled status code "<<msg->status_code);
			mState = Fail;
	}

	osip_message_free(msg);
	return mState;
}


SIPState SIPEngine::MOCSendACK()
{
	assert(mOK);
	SEDCOUT("SIPEngine::MOCSendACK mState=" << mState)
	// make new via branch.
	char tmp[100];
	make_branch(tmp);
	via_branch = tmp;

	// get to tag from mOK message and copy to ack.
	osip_uri_param_t * to_tag_param;
	osip_to_get_tag(mOK->to, &to_tag_param);
	to_tag = to_tag_param->gvalue;

	// get ip owner from mOK message.

	// HACK -- need to fix this crash
	get_owner_ip(mOK, tmp);
	o_addr = tmp;
	osip_message_t * ack;

	// Now ack the OKAY
	// HACK- setting owner address to localhost 
	// since we know its asterisk and get_owner_ip is
	// segfaulting. BUG []
	ack = sip_ack( o_addr.c_str()
		, mCalledUsername.c_str(), 
		sip_username.c_str(), local_port, 
		asterisk_ip.c_str(), asterisk_ip.c_str(), 
		from_tag.c_str(), to_tag.c_str(), 
		via_branch.c_str(), mCallID.c_str(), mCSeq
	);

	gSIPInterface.write(ack);
	osip_message_free(ack);	
	SEDCOUT("SIPEngine::sendACK: Call Active ")

	mState=Active;
	return mState;
}


SIPState SIPEngine::MODSendBYE()
{
	SEDCOUT("SIPEngine::MODSendBYE mState=" << mState);
	char tmp[50];
	make_branch(tmp);
	via_branch = tmp;
	mCSeq++;

	osip_message_t * bye = sip_bye(o_addr.c_str(), mCalledUsername.c_str(), 
		sip_username.c_str(), local_port, asterisk_ip.c_str(), 
		asterisk_ip.c_str(), from_tag.c_str(), to_tag.c_str(), 
		via_branch.c_str(), mCallID.c_str(), mCSeq );

	gSIPInterface.write(bye);
	saveBYE(bye);
	osip_message_free(bye);
	mState = Clearing;
	return mState;
}

SIPState SIPEngine::MODResendBYE()
{
	SEDCOUT("SIPEngine::MODWaitForBYE mState=" << mState);
	assert(mBYE);
	gSIPInterface.write(mBYE);
	return mState;
}

SIPState SIPEngine::MODWaitForOK()
{
	SEDCOUT("SIPEngine::MODWaitForOK mState=" << mState);
	osip_message_t * ok = gSIPInterface.read(mCallID, BYETimeout);
	if(ok->status_code == 200 ) {
		mState = Cleared;
		// Remove Call ID at the end of interaction.
		gSIPInterface.map().remove(mCallID);	
	}

	osip_message_free(ok);
	return mState;	
}



SIPState SIPEngine::MTDCheckBYE()
{
	// Need to check size of osip_message_t* fifo,
	// so need to get fifo pointer and get size.
	// HACK -- reach deap inside to get damn thing
	if (gSIPInterface.fifoSize(mCallID)==0) return mState;	
	if (mState!=Active) return mState;
		
	osip_message_t * msg = gSIPInterface.read(mCallID);
	

	if ((msg->sip_method!=NULL) && (strcmp(msg->sip_method,"BYE")==0)) { 
		SEDCOUT("SIPEngine:::MTDCheckBYE: found msg="<<msg->sip_method)	
		saveBYE(msg);
		mState =  Clearing;
	}

	// FIXME -- Check for repeated ACK and send OK if needed.

	osip_message_free(msg);
	return mState;
} 


SIPState SIPEngine::MTDSendOK()
{
	SEDCOUT("SIPEngine::MTDSendOK mState=" << mState);
	assert(mBYE);
	osip_message_t * okay = sip_b_okay(mBYE);
	gSIPInterface.write(okay);
	osip_message_free(okay);
	mState = Cleared;
	return mState;
}


SIPState SIPEngine::MTCSendTrying()
{
	if (mINVITE==NULL) mState=Fail;
	if (mState==Fail) return mState;
	SEDCOUT("MTCSendTrying mState=" << mState);
	osip_message_t * trying = sip_trying(mINVITE, sip_username.c_str(), asterisk_ip.c_str());
	gSIPInterface.write(trying);
	osip_message_free(trying);
	mState=Proceeding;
	return mState;
}


SIPState SIPEngine::MTCSendRinging()
{
	assert(mINVITE);
	SEDCOUT("MTCSendRinging mState=" << mState);

	// Set the configuration for
	// ack message.	
	char tmp[20];
	make_tag(tmp);
	to_tag = tmp;
		
	SEDCOUT("MTCSendRinging: send ringing")
	osip_message_t * ringing = sip_ringing(mINVITE, 
		sip_username.c_str(), asterisk_ip.c_str(), to_tag.c_str());
	gSIPInterface.write(ringing);
	osip_message_free(ringing);

	mState = Proceeding;
	return mState;
}



SIPState SIPEngine::MTCSendOK( short wrtp_port, unsigned wcodec )
{
	assert(mINVITE);
	rtp_port = wrtp_port;
	codec = wcodec;
	SEDCOUT("MTCSendOK port=" << wrtp_port << " codec=" << codec);
	// Form ack from invite and new parameters.
	osip_message_t * okay = sip_okay(mINVITE, sip_username.c_str(),
		asterisk_ip.c_str(), local_port,to_tag.c_str() , rtp_port, codec);
	gSIPInterface.write(okay);
	osip_message_free(okay);
	mState=Connecting;
	return mState;
}

SIPState SIPEngine::MTCWaitForACK()
{
	// wait for ack,set this to timeout of 
	// of call channel.  If want a longer timeout 
	// period, need to split into 2 handle situation 
	// like MOC where this fxn if called multiple times. 
	SEDCOUT("SIPEngine::MTCWaitForACK: okay sentwaiting for ack ")
	osip_message_t * ack;

	try {
		ack = gSIPInterface.read(mCallID, 1000);
	}
	catch (SIPTimeout& e) {
		SEDCOUT("SIPEngine::MTCWaitForACK timeout") 
		mState = Timeout;	
		return mState;
	}
	catch (SIPError& e) {
		SEDCOUT("SIPEngine::MTCWaitForACK read error");
		mState = Fail;
		return mState;
	}

	if (ack->sip_method==NULL) {
		CERR("SIPEngine::MTCWaitForACK: SIP message with no method"); 
		mState = Fail;
		osip_message_free(ack);
		return mState;	
	}

	SEDCOUT("SIPEngine::MTCWaitForACK: received sip_method="<<ack->sip_method )

	// check for duplicated INVITE
	if( strcmp(ack->sip_method,"INVITE") == 0){ 
		SEDCOUT("SIPEngine::MTCWaitForACK: Received duplicate INVITE ")
	}
	// check for the ACK
	else if( strcmp(ack->sip_method,"ACK") == 0){ 
		SEDCOUT("SIPEngine::MTCWaitForACK: Received ACK ")
		mState=Active;
	}
	// check for the CANCEL
	else if( strcmp(ack->sip_method,"CANCEL") == 0){ 
		SEDCOUT("SIPEngine::MTCWaitForACK: Received CANCEL ")
		mState=Fail;
	}
	// check for strays
	else {
		CERR("SIPEngine::MTCWaitForAck Unexpected Message "<<ack->sip_method) 
		mState = Fail;
	}

	osip_message_free(ack);
	return mState;	
}

void SIPEngine::InitRTP(const osip_message_t * msg )
{
	if(session == NULL)
		session = rtp_session_new(RTP_SESSION_SENDRECV);

	rtp_session_set_blocking_mode(session, TRUE);
	rtp_session_set_scheduling_mode(session, TRUE);

	rtp_session_set_connected_mode(session, TRUE);
	rtp_session_set_symmetric_rtp(session, TRUE);
	// Hardcode RTP session type to GSM full rate (GSM 06.10).
	// FIXME -- Make this work for multiple vocoder types.
	rtp_session_set_payload_type(session, 3);

	char d_ip_addr[20];
	char d_port[10];
	get_rtp_params(msg, d_port, d_ip_addr);
	SEDCOUT("SIPEngine::InitRTP="<<d_ip_addr<<" "<<d_port<<" "<<rtp_port)

	rtp_session_set_local_addr(session, "0.0.0.0", rtp_port );
	rtp_session_set_remote_addr(session, d_ip_addr, atoi(d_port));

}


void SIPEngine::MTCInitRTP()
{
	assert(mINVITE);
	InitRTP(mINVITE);
}


void SIPEngine::MOCInitRTP()
{
	assert(mOK);
	InitRTP(mOK);
}




void SIPEngine::TxFrame( unsigned char * tx_frame ){
	if(mState!=Active) return;
	else {
		// HACK -- Hardcoded for GSM/8000.
		// FIXME -- Make this work for multiple vocoder types.
		rtp_session_send_with_ts(session, tx_frame, 33, tx_time);
		tx_time += 160;		
	}
}


int SIPEngine::RxFrame(unsigned char * rx_frame){
	if(mState!=Active) return 0; 
	else {
		int more;
		int ret=0;
		// HACK -- Hardcoded for GSM/8000.
		// FIXME -- Make this work for multiple vocoder types.
		ret = rtp_session_recv_with_ts(session, rx_frame, 33, rx_time, &more);
		rx_time += 160;
		return ret;
	}	
}



// vim: ts=4 sw=4
