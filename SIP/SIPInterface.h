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



#ifndef SIPINTERFACE_H
#define SIPINTERFACE_H

#include "Interthread.h"
#include "Sockets.h"
#include <osip2/osip.h>




namespace SIP {


class OSIPMessageFIFO : public InterthreadQueue<osip_message_t> {};
class OSIPMessageFIFOMap : public InterthreadMap<std::string,OSIPMessageFIFO> {};


std::ostream& operator<<(std::ostream& os, const OSIPMessageFIFO& m);


/**
	A Map the keeps a SIP message FIFO for each active SIP transaction.
	Keyed by SIP call ID string.
	Overall map is thread-safe.  Each FIFO is also thread-safe.
*/
// FIXME -- This should probably just be a subclass of OSIPMessageFIFOMap.
class SIPMessageMap 
{

private:

	OSIPMessageFIFOMap mMap;

public:

	/** Write sip message to the map+fifo. used by sip interface. */
	void write(const std::string& call_id, osip_message_t * sip_msg );

	/** Read sip message out of map+fifo. used by sip engine. */
	osip_message_t * read(const std::string& call_id, unsigned readTimeout=gBigReadTimeout);
	
	/** Create a new entry in the map. */
	bool add(const std::string& call_id);

	/**
		Remove a fifo from map (called at the end of a sip interaction).
		@param call_id The call_id key string.
		@return True if the call_id was there in the first place.
	*/
	bool remove(const std::string& call_id);

	/** Direct access to the map. */
	// FIXME -- This should probably be replaced with more specific methods.
	OSIPMessageFIFOMap& map() {return mMap;}

};

std::ostream& operator<<(std::ostream& os, const SIPMessageMap& m);




class SIPInterface 
{
	UDPSocket * mSIPSocket;

	char mRemoteIP[100];
	unsigned short mLocalPort;
	unsigned short mRemotePort;
	
	Mutex mSocketLock;
	Thread mDriveThread;	
	SIPMessageMap mSIPMap;	
	
	
	
public:
	// 2 ways to starte sip interface. 
	// Ex 1.
	// SIPInterface si;
	// si.localAdder(port0, ip_str, port1);
	// si.open(); 
	// or Ex 2.
	// SIPInterface si(port0, ip_str, port1);
	// Then after all that. si.start();

	void remoteAddr( unsigned short wRemotePort, const char *wRemoteIP ){ 
		mRemotePort = wRemotePort; 
		strcpy(mRemoteIP, wRemoteIP); 
	}

	void localAddr(unsigned short wLocalPort ){ mLocalPort = wLocalPort; }

	void open()
	{
		mSIPSocket = new UDPSocket( mLocalPort, mRemoteIP, mRemotePort);
	}


	SIPInterface(unsigned short wLocalPort, const char * wRemoteIP, unsigned short wRemotePort )
	{
		mSIPSocket = new UDPSocket(wLocalPort, wRemoteIP, wRemotePort);
	}
		
	
	/** Start the SIP drive loop. */
	void start();

	/** Receive, parse and dispatch a single SIP message. */
	void drive();

	/**
		Look for incoming INVITE messages to start MTC.
		@return true if the message is a new INVITE
	*/
	bool checkInvite( osip_message_t *);

	// To write a msg to outside, make the osip_message_t 
	// then call si.write(msg);
	// to read, you need to have the call_id
	// then call si.read(call_id)

	void write(osip_message_t * msg);

	osip_message_t* read(const std::string& call_id , unsigned readTimeout=gBigReadTimeout)
	{
		return mSIPMap.read(call_id, readTimeout);
	}

	SIPMessageMap& map() { return mSIPMap; }	

	int fifoSize(const std::string& call_id )
	{ 
		OSIPMessageFIFO * fifo = mSIPMap.map().read(call_id);
		if(fifo==NULL) return -1;
		return fifo->size();
	}	
};

void driveLoop(SIPInterface*);


}; // namespace SIP.


/*@addtogroup Globals */
//@{
/** A single global SIPInterface in the global namespace. */
extern SIP::SIPInterface gSIPInterface;
//@}


#endif // SIPINTERFACE_H
// vim: ts=4 sw=4
