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

/*
Contributors:
David A. Burgess, dburgess@ketsrelsp.com
*/

#ifndef SIPENGINE_H
#define SIPENGINE_H

#include <string>
#include <sys/time.h>
#include <sys/types.h>
#include <semaphore.h>

#include <osip2/osip.h>
#include <ortp/ortp.h>


#include "Sockets.h"


namespace SIP {


class SIPInterface;


enum SIPState  {
	NullState,
	Timeout,
	Starting,
	Proceeding,	
	Ringing,
	Busy,
	Connecting,
	Active,
	Clearing,
	Cleared,
	Fail
};


/**@name Timeout values for SIP actions, in ms. */
//@{
const unsigned INVITETimeout = 2000;
const unsigned BYETimeout = 2000;
//@}

std::ostream& operator<<(std::ostream& os, SIPState s);

class SIPEngine 
{

public:

	enum Method { SIPRegister =0, SIPUnregister=1 };

private:
	// Generic Connection Management
	short local_port;
	short asterisk_port;
	std::string asterisk_ip;

	// MOC, MTC information.
	std::string  o_addr;
	short rtp_port;
	std::string mCalledUsername;
	std::string mCalledDomain;
	unsigned codec;

	// General SIP tags and ids.	
	std::string to_tag;
	std::string from_tag;
	std::string via_branch;
	std::string mCallID;
	std::string sip_username;
	unsigned  mCSeq;

	osip_message_t * mINVITE;	///< the INVITE message for this transaction
	osip_message_t * mOK;		///< the INVITE-OK message for this transaction
	osip_message_t * mBYE;		///< the BYE message for this transaction

public:
	
	RtpSession * session;

private:

	SIPState mState;

public:
	
	unsigned int tx_time;
	unsigned int rx_time;
	int time_outs;

	/** Default contructor. Initialize the object. */
	SIPEngine( short w_local_port, short w_asterisk_port, 
	const char * w_asterisk_ip )
		:local_port(w_local_port), 
		asterisk_port(w_asterisk_port), asterisk_ip(w_asterisk_ip),
		mCSeq(random()%1000),
		mINVITE(NULL), mOK(NULL), mBYE(NULL),
		session(NULL), mState(NullState),tx_time(0), rx_time(0)
	{}

	/** Empty constructor. */
	SIPEngine()
		:mCSeq(random()%1000),
		mINVITE(NULL), mOK(NULL), mBYE(NULL),
		session(NULL), mState(NullState),tx_time(0), rx_time(0)
	{}

	/** Destroy held message copies. */
	~SIPEngine();

	const std::string& callID() const { return mCallID; } 

	/** Return the current SIP call state. */
	SIPState state() const { return mState; }

	// will automatically allocate mCallID. 
	// good for mobile originated call and registration.
	void User( const char * w_username );

	// use this for incoming invite message in SIPInterface.
	void User( const char * wCallID, const char * w_username );

	/**@name Messages for SIP registration. */
	//@{

	/**
		Send sip register and look at return msg.
		Can throw SIPTimeout().
		@return True on success.
	*/
	bool Register(Method wMethod=SIPRegister);	

	/**
		Send sip unregister and look at return msg.
		Can throw SIPTimeout().
		@return True on success.
	*/
	bool Unregister() { return (Register(SIPUnregister)); };

	//@}

	
	/**@name Messages associated with MOC procedure. */
	//@{

	/**
		Send an invite message.
		@param called_username SIP userid or E.164 address.
		@param called_domain SIP user's domain.
		@param rtp_port UDP port to use for speech (will use this and next port)
		@param codec Code for codec to be used.
		@return New SIP call state.
	*/
	SIPState MOCSendINVITE(const char * called_username,
		const char * called_domain, short rtp_port, unsigned codec);

	SIPState MOCResendINVITE();

	SIPState MOCWaitForOK();

	SIPState MOCSendACK();
	//@}


	/** Save a copy of an INVITE message in the engine. */
	void saveINVITE(const osip_message_t *invite);

	/** Save a copy of an OK message in the engine. */
	void saveOK(const osip_message_t *OK);

	/** Save a copy of an BYE message in the engine. */
	void saveBYE(const osip_message_t *BYE);


	/**@name Messages associated with MTC procedure. */
	//@{
	SIPState MTCSendTrying();

	SIPState MTCSendRinging();

	SIPState MTCSendOK(short wrtp_port, unsigned wcodec);

	SIPState MTCWaitForACK();
	//@}


	/**@name Messages for MOD procedure. */
	//@{
	SIPState MODSendBYE();

	SIPState MODResendBYE();

	SIPState MODWaitForOK();
	//@}


	/**@name Messages for MTD procedure. */
	//@{
	SIPState MTDCheckBYE();	

	SIPState MTDSendOK();
	//@}


	void FlushRTP(){
//		flushq(&session->rtp.rq, FLUSHALL);
//		flushq(&session->rtp.tev_rq, FLUSHALL);
	}

	void TxFrame( unsigned char * tx_frame );
	int  RxFrame(unsigned char * rx_frame);

	// We need the host sides RTP information contained
	// in INVITE or 200_OKAY
	void InitRTP(const osip_message_t * msg );
	void MOCInitRTP();
	void MTCInitRTP();

};


}; 

#endif // SIPENGINE_H
// vim: ts=4 sw=4
