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




#include <ortp/ortp.h>
#include <osipparser2/sdp_message.h>

#include "GSMConfig.h"
#include "ControlCommon.h"

#include "Sockets.h"

#include "SIPUtility.h"
#include "SIPInterface.h"

#include <Logger.h>



using namespace std;
using namespace SIP;

using namespace GSM;
using namespace Control;



// SIPMessageMap method definitions.

void SIPMessageMap::write(const std::string& call_id, osip_message_t * msg)
{
	LOG(DEBUG) << "SIPMessageMap::write " << call_id << " msg " << msg;
	OSIPMessageFIFO * fifo = mMap.readNoBlock(call_id);
	if( fifo==NULL ) {
		LOG(NOTICE) << "write missing SIP FIFO "<<call_id;
		throw SIPError();
	}
	LOG(DEBUG) << "SIPMessage::write on fifo " << fifo;
	fifo->write(msg);	
}

osip_message_t * SIPMessageMap::read(const std::string& call_id, unsigned readTimeout)
{ 
	LOG(DEBUG) << "SIPMessageMap::read " << call_id;
	OSIPMessageFIFO * fifo = mMap.readNoBlock(call_id);
	if( fifo == NULL ) {
		LOG(NOTICE) << "read missing SIP FIFO "<<call_id;
		throw SIPError();
	}	
	LOG(DEBUG) << "SIPMessageMap::read blocking on fifo " << fifo;
	osip_message_t * msg =  fifo->read(readTimeout);	
	if( msg == NULL ) throw SIPTimeout();
	return msg;
}


bool SIPMessageMap::add(const std::string& call_id, const struct sockaddr_in* returnAddress)
{
	OSIPMessageFIFO * fifo = new OSIPMessageFIFO(returnAddress);
	mMap.write(call_id, fifo);
	return true;
}

bool SIPMessageMap::remove(const std::string& call_id)
{
	OSIPMessageFIFO * fifo = mMap.readNoBlock(call_id);
	if(fifo == NULL) return false;
	mMap.remove(call_id);
	return true;
}





// SIPInterface method definitions.

bool SIPInterface::addCall(const string &call_id)
{
	LOG(INFO) << "creating SIP message FIFO callID " << call_id;
	return mSIPMap.add(call_id,mSIPSocket.source());
}


bool SIPInterface::removeCall(const string &call_id)
{
	LOG(INFO) << "removing SIP message FIFO callID " << call_id;
	return mSIPMap.remove(call_id);
}

int SIPInterface::fifoSize(const std::string& call_id )
{ 
	OSIPMessageFIFO * fifo = mSIPMap.map().read(call_id,0);
	if(fifo==NULL) return -1;
	return fifo->size();
}	



SIPInterface::SIPInterface()
	:mSIPSocket(gConfig.getNum("SIP.Port"), gConfig.getStr("Asterisk.IP"), gConfig.getNum("Asterisk.Port"))
{
	mAsteriskPort = gConfig.getNum("Asterisk.Port");
	mMessengerPort = gConfig.getNum("Messenger.Port");
	assert(resolveAddress(&mAsteriskAddress,gConfig.getStr("Asterisk.IP"),mAsteriskPort));
	assert(resolveAddress(&mMessengerAddress,gConfig.getStr("Messenger.IP"),mMessengerPort));
}


void SIP::driveLoop( SIPInterface * si){
	while (true) {
		si->drive();
	}
}

void SIPInterface::start(){
	// Start all the osip/ortp stuff. 
	parser_init();
	ortp_init();
	ortp_scheduler_init();
	// FIXME -- Can we coordinate this with the global logger?
	//ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
	mDriveThread.start((void *(*)(void*))driveLoop,this );
}




void SIPInterface::write(const struct sockaddr_in* dest, osip_message_t *msg) 
{
	char * str;
	size_t msgSize;
	osip_message_to_str(msg, &str, &msgSize);
	LOG(DEBUG) << "SIPInterface::write:\n"<< str;
	mSocketLock.lock();
	mSIPSocket.send((const struct sockaddr*)dest,str);
	mSocketLock.unlock();
	free(str);
}



void SIPInterface::drive() 
{
	char buffer[2048];

	LOG(DEBUG) << "SIPInterface::drive() blocking on socket";
	mSIPSocket.read(buffer);
	LOG(DEBUG) << "SIPInterface::drive() read " << buffer;

	try {

		// Parse the mesage.
		osip_message_t * msg;
		osip_message_init(&msg);
		osip_message_parse(msg, buffer, strlen(buffer));
	
		if (msg->sip_method!=NULL) {
			LOG(DEBUG) << "drive() read method " << msg->sip_method;
		} else {
			LOG(DEBUG) << "drive() read (no method)";
		}
	
		// Must check if msg is an invite.
		// if it is, handle appropriatly.
		// FIXME -- Check return value in case this failed.
		checkInvite(msg);

		// FIXME -- Need to check for early BYE to stop paging .
		// If it's a BYE, find the corresponding transaction table entry.
		// If the Q931 state is "paging", or T3113 is expired, remove it.
		// Otherwise, we keep paging even though the call has ended.
		
		// Multiplex out the received SIP message to active calls.

		// If we write to non-existent call_id.
		// this is errant message so need to catch
		// Internal error excatpion. and give nice
		// message (instead of aborting)
		// Don't free call_id_num.  It points into msg->call_id.
		char * call_id_num = osip_call_id_get_number(msg->call_id);	
		if( call_id_num == NULL ) {
			LOG(WARN) << "SIPInterface::drive message with no call id";
			throw SIPError();
		}
		LOG(DEBUG) << "drive: got message " << msg << " with call id " << call_id_num << " and writing it to the map.";
		string call_num(call_id_num);
		mSIPMap.write(call_num, msg);
	}
	catch(SIPException) {
		LOG(WARN) << "Errant SIP Message: "<<buffer;
	}
}




bool SIPInterface::checkInvite( osip_message_t * msg )
{
	LOG(DEBUG) << "SIPInterface::checkInvite";

	// Is there even a message?
	if(!msg->sip_method) return false;

	// Check for INVITE or MESSAGE methods.
	GSM::ChannelType requiredChannel;
	bool channelAvailable = false;
	GSM::L3CMServiceType serviceType;
	if (strcmp(msg->sip_method,"INVITE") == 0) {
		// INVITE is for MTC.
		// Set the required channel type to match the assignment style.
		switch ((AssignmentType)gConfig.getNum("GSM.AssignmentType")) {
			case GSM::EarlyAssignment:
					requiredChannel = GSM::SDCCHType;
					channelAvailable = gBTS.SDCCHAvailable() && gBTS.TCHAvailable();
					break;
			case GSM::VeryEarlyAssignment:
					requiredChannel = GSM::TCHFType;
					channelAvailable = gBTS.TCHAvailable();
					break;
			default:
				LOG(ERROR) << "GSM.AssignmentType " << gConfig.getStr("GSM.AssignmentType") << " not defined";
				assert(0);
		}
		serviceType = L3CMServiceType::MobileTerminatedCall;
	}
	else if (strcmp(msg->sip_method,"MESSAGE") == 0) {
		// MESSAGE is for MTSMS.
		requiredChannel = GSM::SDCCHType;
		channelAvailable = gBTS.SDCCHAvailable();
		serviceType = L3CMServiceType::MobileTerminatedShortMessage;
	}
	else {
		return false;
	}

	LOG(INFO) << "set up MTC paging for channel=" << requiredChannel << " available=" << channelAvailable;
	// Check gBTS for channel availability.
	if (!channelAvailable) {
		// FIXME -- Send 480 "Temporarily Unavailable" response on SIP interface.
		// FIXME -- We need this for SMS.
		LOG(NOTICE) << "MTC CONGESTION, no channel availble for assignment";
		return false;
	}

	// Get call_id from invite message.
	if (!msg->call_id) {
		// FIXME -- Send appropriate error on SIP interface.
		LOG(WARN) << "Incoming INVITE/MESSAGE with no valid call ID";
		return false;
	}

	// Don't free call_id_num.  It points into msg->call_id.
	char * call_id_num = osip_call_id_get_number(msg->call_id);	
	string call_id_string(call_id_num);

	// Get sip_username (IMSI) from invite. 
	// typical to_uri: sip:310410186585289@192.168.1.3:5050
	// IMSI:310410186585289 
	// FIXME -- This will CRASH if we receive a malformed INVITE.
	char * to_uri;
	osip_uri_to_str(msg->to->url, &to_uri);	
	LOG(DEBUG) << "SIPInterface::checkInvite: " << msg->sip_method << " to "<< to_uri;
	char * IMSI = strtok(&to_uri[4],"@");
	// Make the mobile id we need for transaction and paging enties.
	L3MobileIdentity mobile_id(IMSI);

	// Check SIP map.  Repeated entry?  Page again.
	if (mSIPMap.map().readNoBlock(call_id_string) != NULL) { 
		TransactionEntry transaction;
		if (!gTransactionTable.find(mobile_id,transaction)) {
			// FIXME -- Send "call leg non-existent" response on SIP interface.
			LOG(WARN) << "repeated INVITE/MESSAGE with no transaction record";
			return false;
		}
		LOG(DEBUG) << "SIPInterface::checkInvite: repeated SIP INVITE/MESSAGE, repaging"; 
		gBTS.pager().addID(mobile_id,requiredChannel,transaction.ID());	
		transaction.T3113().set();
		gTransactionTable.update(transaction);
		osip_free(to_uri);
		return false;
	}

	// Add an entry to the SIP Map to route inbound SIP messages.
	//mSIPMap.add(call_id_string,mSIPSocket.source());
	addCall(call_id_string);

	// Install transaction.
	LOG(DEBUG) << "SIPInterface::checkInvite: make new transaction ";
	// Put the caller ID in here if it's available.
	const char emptyString[] = "";
	const char *callerID = NULL;
	osip_from_t *from = osip_message_get_from(msg);
	if (from) {
		osip_uri_t* url = osip_contact_get_url(from);
		if (url) callerID = url->username;
	} else {
		LOG(NOTICE) << "INVITE with no From: username";
	}
	if (!callerID) callerID = emptyString;
	LOG(DEBUG) << "SIPInterface: callerID \"" << callerID << "\"";
	// Build the transaction table entry.
	TransactionEntry transaction(mobile_id,serviceType,callerID);
	transaction.T3113().set();
	transaction.Q931State(TransactionEntry::Paging);
	LOG(DEBUG) << "SIPInterface: call_id_num \"" << call_id_num << "\"";
	LOG(DEBUG) << "SIPInterface: IMSI \"" << IMSI << "\"";


	transaction.SIP().User(call_id_num,IMSI,callerID);
	transaction.SIP().saveINVITE(msg);
	if (serviceType == L3CMServiceType::MobileTerminatedShortMessage) {
		osip_body_t *body;
		osip_message_get_body(msg,0,&body);
		char *text;
		size_t length;
		osip_body_to_str(body,&text,&length);
		if (text) transaction.message(text);
		else LOG(NOTICE) << "incoming MESSAGE method with no message body";
	}
	LOG(DEBUG) << "SIPInterface::checkInvite: making transaction and add to transaction table: "<< transaction;
	gTransactionTable.add(transaction); 
	
	// Add to paging list.
	LOG(DEBUG) << "SIPInterface::checkInvite: new SIP invite, initial paging for mobile ID " << mobile_id;
	gBTS.pager().addID(mobile_id,requiredChannel,transaction.ID());	

	osip_free(to_uri);
	return true;
}





// vim: ts=4 sw=4
