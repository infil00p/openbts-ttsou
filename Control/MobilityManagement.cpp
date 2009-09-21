/**@file GSM/SIP Mobility Management, GSM 04.08. */
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






#include "ControlCommon.h"
#include "GSMLogicalChannel.h"
#include "GSML3RRMessages.h"
#include "GSML3MMMessages.h"
#include "GSML3CCMessages.h"
#include "GSMConfig.h"

using namespace std;

#include "SIPInterface.h"
#include "SIPUtility.h"
#include "SIPMessage.h"
#include "SIPEngine.h"
using namespace SIP;


using namespace GSM;
using namespace Control;





/** Controller for CM Service requests, dispatches out to multiple possible transaction controllers. */
void Control::CMServiceResponder(const L3CMServiceRequest* cmsrq, LogicalChannel* DCCH)
{
	assert(cmsrq);
	assert(DCCH);
	CLDCOUT("CMServiceResponder " << *cmsrq);
	switch (cmsrq->serviceType().type()) {
		case L3CMServiceType::MobileOriginatedCall:
			MOCStarter(cmsrq,DCCH);
			break;
		default:
			CLDCOUT("CMServiceResponder service not supported");
			// Cause 0x20 means "serivce not supported".
			DCCH->send(L3CMServiceReject(0x20));
			DCCH->send(L3ChannelRelease());
	}
}




/** Controller for the IMSI Detach transaction, GSM 04.08 4.3.4. */
void Control::IMSIDetachController(const L3IMSIDetachIndication* idi, SDCCHLogicalChannel* SDCCH)
{
	assert(idi);
	assert(SDCCH);
	CLDCOUT("IMSIDetachController " << *idi);

	// The IMSI detach maps to a SIP unregister with the local Asterisk server.
	try { 
		// FIXME -- Resolve TMSIs to IMSIs.
		if (idi->mobileIdentity().type()==IMSIType) {
			SIPEngine engine;
			engine.User(idi->mobileIdentity().digits());
			engine.Unregister();
		}
	}
	catch(SIPTimeout) {
		CERR("WARNING -- SIP registration timed out.  Is Asterisk running?");
	}
	// No reponse required, so just close the channel.
	SDCCH->send(L3ChannelRelease());
	// Many handsets never complete the transaction.
	// So force a shutdown of the channel.
	SDCCH->send(ERROR);
}


/**
	Controller for the Location Updating transaction, GSM 04.08 4.4.4.
	@param lur The location updating request.
	@param SDCCH The Dm channel to the MS, which will be released by the function.
*/
void Control::LocationUpdatingController(const L3LocationUpdatingRequest* lur, SDCCHLogicalChannel* SDCCH)
{
	assert(SDCCH);
	assert(lur);
	CLDCOUT("LocationUpdatingController " << *lur);

	// The location updating request gets mapped to a SIP
	// registration with the local Asterisk server.
	// If the registration is successful, we may assign a new TMSI.

	// Reject causes come from GSM 04.08 10.5.3.6
	// Be careful what cause you send, since some of
	// them (like the "invalid MS/ME" codes) can leave the phone
	// locked up until the battery is removed.

	// Resolve an IMSI and see if there's a pre-existing IMSI-TMSI mapping.
	L3MobileIdentity mobID = lur->mobileIdentity();
	bool sameLAI = (lur->LAI() == gBTS.LAI());
	unsigned assignedTMSI = resolveIMSI(sameLAI,mobID,SDCCH);

	// Try to register the IMSI with Asterisk.
	// This will be set true if registration succeeded in the SIP world.
	bool success = false;
	try {
		SIPEngine engine;
		engine.User(mobID.digits());
		CLDCOUT("LocationUpdatingController waiting for registration");
		success = engine.Register(); 
	}
	catch(SIPTimeout) {
		CERR("WARNING -- SIP registration timed out.  Is Asterisk running?");
		// Reject with a "network failure" cause code, 0x11.
		SDCCH->send(L3LocationUpdatingReject(0x11));
		// Release the channel and return.
		SDCCH->send(L3ChannelRelease());
		return;
	}

	if (!success) {
		CLDCOUT("LocationUpdatingController : SIPRegistration -> FAILED")
		// Reject cause 0x03, IMSI not in VLR
		SDCCH->send(L3LocationUpdatingReject(0x03));
		// Release the channel and return.
		SDCCH->send(L3ChannelRelease());
		return;
	}

	// If we are here, registration was successful.

	CLDCOUT("LocationUpdatingController : SIPRegistration -> SUCCESS")
	// Send the "short name".
	// TODO -- Set the handset clock in this message, too.
	SDCCH->send(L3MMInformation(gBTS.shortName()));

	// Accept. Need a TMSI assignment, too?
	if (assignedTMSI) SDCCH->send(L3LocationUpdatingAccept(gBTS.LAI()));
	else SDCCH->send(L3LocationUpdatingAccept(gBTS.LAI(),gTMSITable.assign(mobID.digits())));

	// Close the channel.
	SDCCH->send(L3ChannelRelease());
}





// vim: ts=4 sw=4
