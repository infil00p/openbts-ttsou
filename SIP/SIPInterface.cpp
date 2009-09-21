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




using namespace std;
using namespace SIP;

using namespace GSM;
using namespace Control;



// SIPMessageMap method definitions.

void SIPMessageMap::write(const std::string& call_id, osip_message_t * msg)
{
	DCOUT("SIPMessageMap::write " << call_id << " msg " << msg);
	OSIPMessageFIFO * fifo = mMap.readNoBlock(call_id);
	if( fifo==NULL ) {
		CERR("WARNING -- write missing SIP FIFO ("<<call_id<<")");
		throw SIPError();
	}
	DCOUT("SIPMessage::write on fifo " << fifo);
	fifo->write(msg);	
}

osip_message_t * SIPMessageMap::read(const std::string& call_id, unsigned readTimeout)
{ 
	DCOUT("SIPMessageMap::read " << call_id);
	OSIPMessageFIFO * fifo = mMap.readNoBlock(call_id);
	if( fifo == NULL ) {
		CERR("WARNING -- read missing SIP FIFO ("<<call_id<<")");
		throw SIPError();
	}	
	DCOUT("SIPMessageMap::read blocking on fifo " << fifo);
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
	//ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR);
	mDriveThread.start((void *(*)(void*))driveLoop,this );
}




void SIPInterface::write(const struct sockaddr_in* dest, osip_message_t *msg) 
{
	char * str;
	size_t msgSize;
	osip_message_to_str(msg, &str, &msgSize);
	DCOUT("SIPInterface::write:\n"<< str)
	mSocketLock.lock();
	mSIPSocket.send((const struct sockaddr*)dest,str);
	mSocketLock.unlock();
	free(str);
}



void SIPInterface::drive() 
{
	char buffer[2048];

	DCOUT("SIPInterface::drive() blocking on socket");
	mSIPSocket.read(buffer);
	DCOUT("SIPInterface::drive() read " << buffer<<"\n");

	try {

		// Parse the mesage.
		osip_message_t * msg;
		osip_message_init(&msg);
		osip_message_parse(msg, buffer, strlen(buffer));
	
		if (msg->sip_method!=NULL) {
			DCOUT("drive() read method " << msg->sip_method);
		} else {
			DCOUT("drive() read (no method)");
		}
	
		// Must check if msg is an invite.
		// if it is, handle appropriatly.
		checkInvite(msg);

		// FIXME -- Need to check for early BYE to stop paging .
		// If it's a BYE, find the corresponding transaction table entry.
		// If the Q931 state is "paging", or T3113 is expired, remove it.
		// Otherwise, we keep paging even though the call has ended.
		
		// If we write to non-existent call_id.
		// this is errant message so need to catch
		// Internal error excatpion. and give nice
		// message (instead of aborting)
		// Don't free call_id_num.  It points into msg->call_id.
		char * call_id_num = osip_call_id_get_number(msg->call_id);	
		if( call_id_num == NULL ) {
			DCOUT("drive: message with no call id");
			throw SIPError();
		}
		DCOUT("drive: got message " << msg << " with call id " << call_id_num << " and writing it to the map.");
		string call_num(call_id_num);
		mSIPMap.write(call_num, msg);
	}
	catch(SIPException) {
		CERR("WARNING --  Errant SIP Message. "<<buffer)
	}
}




bool SIPInterface::checkInvite( osip_message_t * msg )
{
	DCOUT("SIPInterface::checkInvite")

	// Is there even a message?
	if(!msg->sip_method) return false;

	// Check for INVITE or MESSAGE methods.
	GSM::ChannelType requiredChannel;
	GSM::L3CMServiceType serviceType;
	if (strcmp(msg->sip_method,"INVITE") == 0) {
		// INVITE is for MTC.
		requiredChannel = gINVITEChannel;
		serviceType = L3CMServiceType::MobileTerminatedCall;
	}
	else if (strcmp(msg->sip_method,"MESSAGE") == 0) {
		// MESSAGE is for MTSMS.
		requiredChannel = GSM::SDCCHType;
		serviceType = L3CMServiceType::MobileTerminatedShortMessage;
	}
	else {
		return false;
	}

	// FIXME -- Check gBTS for channel availability.
	// Respond with a congestion message if none are available.

	// Get call_id from invite message.
	if (!msg->call_id) {
		CERR("WARNING -- Incoming INVITE/MESSAGE with no valid call ID");
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
	DCOUT("SIPInterface::checkInvite: " << msg->sip_method << " to "<< to_uri);
	char * IMSI = strtok(&to_uri[4],"@");
	// Make the mobile id we need for transaction and paging enties.
	L3MobileIdentity mobile_id(IMSI);

	// Check SIP map.  Repeated entry?  Page again.
	if (mSIPMap.map().readNoBlock(call_id_string) != NULL) { 
		TransactionEntry transaction;
		if (!gTransactionTable.find(mobile_id,transaction)) {
			CERR("WARNING -- repeated INVITE/MESSAGE with no transaction record");
			return false;
		}
		DCOUT("SIPInterface::checkInvite: repeated SIP INVITE/MESSAGE, repaging") 
		gBTS.pager().addID(mobile_id,requiredChannel);	
		transaction.T3113().set();
		gTransactionTable.update(transaction);
		osip_free(to_uri);
		return false;
	}

	// Add an entry to the SIP Map to route inbound SIP messages.
	mSIPMap.add(call_id_string,mSIPSocket.source());

	// Install transaction.
	DCOUT("SIPInterface::checkInvite: make new transaction ")
	// Put the caller ID in here if it's available.
	const char emptyString[] = "";
	const char *callerID = NULL;
	osip_from_t *from = osip_message_get_from(msg);
	if (from) {
		osip_uri_t* url = osip_contact_get_url(from);
		if (url) callerID = url->username;
	} else {
		DCOUT("NOTICE -- INVITE with no From: username");
	}
	if (!callerID) callerID = emptyString;
	DCOUT("SIPInterface: callerID \"" << callerID << "\"");
	// Build the transaction table entry.
	TransactionEntry transaction(mobile_id,serviceType,callerID);
	transaction.T3113().set();
	transaction.Q931State(TransactionEntry::Paging);
	transaction.SIP().User(call_id_num,IMSI);
	transaction.SIP().saveINVITE(msg);
	if (serviceType == L3CMServiceType::MobileTerminatedShortMessage) {
		osip_body_t *body;
		osip_message_get_body(msg,0,&body);
		char *text;
		size_t length;
		osip_body_to_str(body,&text,&length);
		if (text) transaction.message(text);
		else DCOUT("NOTICE -- incoming MESSAGE method with no message body");
	}
	DCOUT("SIPInterface::checkInvite: making transaction and add to transaction table: "<< transaction);
	gTransactionTable.add(transaction); 
	
	// Add to paging list.
	DCOUT("SIPInterface::checkInvite: new SIP invite, initial paging for mobile ID " << mobile_id);
	gBTS.pager().addID(mobile_id,requiredChannel);	

	osip_free(to_uri);
	return true;
}





// vim: ts=4 sw=4
