/**@file GSM/SIP Mobility Management, GSM 04.08. */
/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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
Raffi Sevlian, raffisev@gmail.com
*/



//#define LOCAL


#include "ControlCommon.h"
#include "GSMLogicalChannel.h"
#include "GSML3RRMessages.h"
#include "GSML3MMMessages.h"
#include "GSML3CCMessages.h"
#include "GSMConfig.h"

using namespace std;

#ifndef LOCAL
#include "SIPInterface.h"
#include "SIPUtility.h"
#include "SIPMessage.h"
#include "SIPEngine.h"
using namespace SIP;
#endif


using namespace GSM;
using namespace Control;





/** Controller for CM Service requests, dispatches out to multiple possible transaction controllers. */
void Control::CMServiceResponder(const L3CMServiceRequest* cmsrq, SDCCHLogicalChannel* SDCCH)
{
	assert(cmsrq);
	CLDCOUT("CMServiceResponder " << *cmsrq);
	switch (cmsrq->serviceType().type()) {
		case L3CMServiceType::MobileOriginatedCall:
			MOCStarter(cmsrq,SDCCH);
			break;
#ifdef SMS
		case L3CMServiceType::ShortMessage:
			ShortMessageServiceStarter(cmsrq, SDCCH);
			break;
#endif
		default:
			CLDCOUT("CMServiceResponder service not supported");
			// Cause 0x20 means "serivce not supported".
			SDCCH->send(L3CMServiceReject(0x20));
			SDCCH->send(L3ChannelRelease());
	}
}




/** Controller for the IMSI Detach transaction, GSM 04.08 4.3.4. */
void Control::IMSIDetachController(const L3IMSIDetachIndication* idi, SDCCHLogicalChannel* SDCCH)
{
	assert(idi);
	CLDCOUT("IMSIDetachController " << *idi);

#ifndef LOCAL
	// The IMSI detach maps to a SIP unregister with the local Asterisk server.
	try { 
		if (idi->mobileIdentity().type()==IMSIType) {
			SIPEngine engine(SIP_UDP_PORT, 5060, "127.0.0.1");
			engine.User(idi->mobileIdentity().digits());
			engine.Unregister();
		}
	}
	catch(SIPTimeout) {
		CERR("IMSIDetachController: SIP TimedOut ");
	}
#endif
	// No reponse required, so just close the channel.
	SDCCH->send(L3ChannelRelease());
	// Mant handsets never complete the transaction.
	// So force a shutdown of the channel.
	SDCCH->send(ERROR);
}



/** Controller for the Location Updating transaction, GSM 04.08 4.4.4. */
void Control::LocationUpdatingController(const L3LocationUpdatingRequest* lur, SDCCHLogicalChannel* SDCCH)
{
	assert(lur);
	CLDCOUT("LocationUpdatingController " << *lur);
	bool success = false;

#ifndef LOCAL
	// The location updating request gets mapped to a SIP
	// registration with the local Asterisk server.
	try {
		if (lur->mobileIdentity().type()==IMSIType) {
			SIPEngine engine(SIP_UDP_PORT, 5060, "127.0.0.1");
			engine.User(lur->mobileIdentity().digits());
			CLDCOUT("LocationUpdatingController waiting for registration");
			success = engine.Register(); 
		}
		// FIXME -- We need an appropriate response if the phone sent a non-IMSI ID.
	}
	catch(SIPTimeout) {
		CLDCOUT("LocationUpdatingController : SIP registration timed out");
		CERR("WARNING -- SIP registration timed out.  Is Asterisk running?");
	}
#else
	// For "local" test, assume Asterisk returns true.
	success = true;
#endif

	if(success){
		// Success.
		CLDCOUT("LocationUpdatingController : SIPRegistration -> SUCCESS")
		SDCCH->send(L3MMInformation(gBTS.shortName()));
		SDCCH->send(L3LocationUpdatingAccept(gBTS.LAI()));
	}else{
		// Failure.
		CLDCOUT("LocationUpdatingController : SIPRegistration -> FAILED")
		// These reject causes come from GSM 04.08 10.5.3.6
		// Be careful what cause you send, since some of
		// them (like the "stolen" code) can leave the phone
		// locked up until the battery is removed.
		// Reject cause 0x03, IMSI not in VLR
		SDCCH->send(L3LocationUpdatingReject(0x03));
	}
	SDCCH->send(L3ChannelRelease());
}





// vim: ts=4 sw=4
