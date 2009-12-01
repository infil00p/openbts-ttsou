/**@file GSM/SIP Mobility Management, GSM 04.08. */
/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
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
	LOG(INFO) << *cmsrq;
	switch (cmsrq->serviceType().type()) {
		case L3CMServiceType::MobileOriginatedCall:
			MOCStarter(cmsrq,DCCH);
			break;
		case L3CMServiceType::ShortMessage:
			MOSMSController(cmsrq,DCCH);
			break;
		case L3CMServiceType::EmergencyCall:
			EmergencyCall(cmsrq,DCCH);
			break;
		default:
			LOG(NOTICE) << "service not supported";
			// Cause 0x20 means "serivce not supported".
			DCCH->send(L3CMServiceReject(0x20));
			DCCH->send(L3ChannelRelease());
	}
	// The transaction may or may not be cleared,
	// depending on the assignment type.
}




/** Controller for the IMSI Detach transaction, GSM 04.08 4.3.4. */
void Control::IMSIDetachController(const L3IMSIDetachIndication* idi, LogicalChannel* DCCH)
{
	assert(idi);
	assert(DCCH);
	LOG(INFO) << *idi;

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
		LOG(ALARM) "SIP registration timed out.  Is Asterisk running?";
	}
	// No reponse required, so just close the channel.
	DCCH->send(L3ChannelRelease());
	// Many handsets never complete the transaction.
	// So force a shutdown of the channel.
	DCCH->send(HARDRELEASE);
}




/**
	Send a given welcome message from a given short code.
	@return true if it was sent
*/
bool sendWelcomeMessage(const char* messageName, const char* shortCodeName, SDCCHLogicalChannel* SDCCH)
{
	if (!gConfig.defines(messageName)) return false;
	LOG(INFO) << "sending " << messageName << " message to handset";
	// This returns when delivery is acked in L3.
	deliverSMSToMS(
		gConfig.getStr(shortCodeName),
		gConfig.getStr(messageName),
		random()%7,SDCCH);
	return true;
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
	LOG(INFO) << *lur;

	// The location updating request gets mapped to a SIP
	// registration with the Asterisk server.
	// If the registration is successful, we may assign a new TMSI.

	// Resolve an IMSI and see if there's a pre-existing IMSI-TMSI mapping.
	// This operation will throw an exception, caught in a higher scope,
	// if it fails in the GSM domain.
	L3MobileIdentity mobID = lur->mobileIdentity();
	bool sameLAI = (lur->LAI() == gBTS.LAI());
	unsigned assignedTMSI = resolveIMSI(sameLAI,mobID,SDCCH);
	// IMSIAttach set to true if this is a new registration.
	bool IMSIAttach = (assignedTMSI==0);
	// Try to register the IMSI with Asterisk.
	// This will be set true if registration succeeded in the SIP world.
	bool success = false;
	try {
		SIPEngine engine;
		engine.User(mobID.digits());
		LOG(DEBUG) << "waiting for registration";
		success = engine.Register(); 
	}
	catch(SIPTimeout) {
		LOG(ALARM) "SIP registration timed out.  Is Asterisk running?";
		// Reject with a "network failure" cause code, 0x11.
		SDCCH->send(L3LocationUpdatingReject(0x11));
		// Release the channel and return.
		SDCCH->send(L3ChannelRelease());
		return;
	}

	// This allows us to configure Open Registration
	bool openRegistration = false;
	if (gConfig.defines("Control.OpenRegistration")) {
		openRegistration = gConfig.getNum("Control.OpenRegistration");
	}

	// We fail closed unless we're configured otherwise
	if (!success && !openRegistration) {
		LOG(INFO) << "registration FAILED: " << mobID;
		SDCCH->send(L3LocationUpdatingReject(gConfig.getNum("GSM.LURejectCause")));
		sendWelcomeMessage( "Control.FailedRegistrationWelcomeMessage",
			"Control.FailedRegistrationWelcomeShortCode", SDCCH);
	}

	// If success is true, we had a normal registration.
	// Otherwise, we are here because of open registration.
	// Either way, we're going to register a phone if we arrive here.

	if (success) LOG(INFO) << "registration SUCCESS: " << mobID;
	else LOG(INFO) << "registration ALLOWED: " << mobID;


	// Send the "short name".
	// TODO -- Set the handset clock in this message, too.
	SDCCH->send(L3MMInformation(gBTS.shortName()));
	// Accept. Make a TMSI assignment, too, if needed.
	if (assignedTMSI) SDCCH->send(L3LocationUpdatingAccept(gBTS.LAI()));
	else SDCCH->send(L3LocationUpdatingAccept(gBTS.LAI(),gTMSITable.assign(mobID.digits())));
	// If this is an IMSI attach, send a welcome message.
	if (IMSIAttach) {
		if (success) {
			sendWelcomeMessage( "Control.NormalRegistrationWelcomeMessage",
				"Control.NormalRegistrationWelcomeShortCode", SDCCH);
		} else {
			sendWelcomeMessage( "Control.OpenRegistrationWelcomeMessage",
				"Control.OpenRegistrationWelcomeShortCode", SDCCH);
		}
	}

	// Release the channel and return.
	SDCCH->send(L3ChannelRelease());
	return;
}





// vim: ts=4 sw=4
