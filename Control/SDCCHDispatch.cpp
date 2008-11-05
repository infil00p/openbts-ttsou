/**@file Idle-mode dispatcher for SDCCH. */

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
*/



#include "ControlCommon.h"
#include "GSMLogicalChannel.h"
#include "GSML3MMMessages.h"
#include "GSML3RRMessages.h"

#define DEBUG 1

using namespace std;
using namespace GSM;
using namespace Control;


/**
	Dispatch the appropriate controller for a Mobility Management message.
	@param req A pointer to the initial message.
	@param SDCCH A pointer to the logical channel for the transaction.
*/
void SDCCHDispatchMM(const L3MMMessage* req, SDCCHLogicalChannel *SDCCH)
{
	assert(req);
	L3MMMessage::MessageType MTI = (L3MMMessage::MessageType)req->MTI();
	switch (MTI) {
		case L3MMMessage::LocationUpdatingRequest:
			LocationUpdatingController(dynamic_cast<const L3LocationUpdatingRequest*>(req),SDCCH);
			break;
		case L3MMMessage::IMSIDetachIndication:
			IMSIDetachController(dynamic_cast<const L3IMSIDetachIndication*>(req),SDCCH);
			break;
		case L3MMMessage::CMServiceRequest:
			CMServiceResponder(dynamic_cast<const L3CMServiceRequest*>(req),SDCCH);
			break;
		default:
			CERR("NOTICE -- (ControlLayer) unhandled MM message " << MTI << " on SDCCH");
			throw UnsupportedMessage();
	}
}


/**
	Dispatch the appropriate controller for a Radio Resource message.
	@param req A pointer to the initial message.
	@param SDCCH A pointer to the logical channel for the transaction.
*/
void SDCCHDispatchRR(const L3RRMessage* req, SDCCHLogicalChannel *SDCCH)
{
	CLDCOUT("SDCCHDispatchRR: cheking MTI"<< (L3RRMessage::MessageType)req->MTI() )

	assert(req);
	L3RRMessage::MessageType MTI = (L3RRMessage::MessageType)req->MTI();
	switch (MTI) {
		case L3RRMessage::PagingResponse:
			PagingResponseHandler(dynamic_cast<const L3PagingResponse*>(req),SDCCH);
			break;
		default:
			CERR("NOTICE -- (ControlLayer) unhandled RR message " << MTI << " on SDCCH");
			throw UnsupportedMessage();
	}
}






/** Example of a closed-loop, persistent-thread control function for the SDCCH. */
void Control::SDCCHDispatcher(SDCCHLogicalChannel *SDCCH)
{
	while (1) {
		try {
			// Wait for a transaction to start.
			CLDCOUT("SDCCHDisptacher waiting for ESTABLISH");
			while (!waitForPrimitive(SDCCH,ESTABLISH)) {}
			// Pull the first message and dispatch a new transaction.
			const L3Message *message = getMessage(SDCCH);
			CLDCOUT("SDCCHDispatcher got " << *message);
			// Each protocol has it's own sub-dispatcher.
			switch (message->PD()) {
				case L3MobilityManagementPD:
					SDCCHDispatchMM(dynamic_cast<const L3MMMessage*>(message),SDCCH);
					break;
				case L3RadioResourcePD:
					SDCCHDispatchRR(dynamic_cast<const L3RRMessage*>(message),SDCCH);
					break;
				default:
					CERR("NOTICE -- (ControlLayer) unhandled protocol " 
						<< message->PD() << " on SDCCH");
					throw UnsupportedMessage();
			}
			delete message;
		}

		catch (ChannelReadTimeout except) {
			// Cause 0x03 means "abnormal release, timer expired".
			SDCCH->send(L3ChannelRelease(0x03));
		}
		catch (UnexpectedPrimitive except) {
			// Cause 0x62 means "message type not not compatible with protocol state".
			SDCCH->send(L3ChannelRelease(0x62));
		}
		catch (UnexpectedMessage except) {
			// Cause 0x62 means "message type not not compatible with protocol state".
			SDCCH->send(L3ChannelRelease(0x62));
		}
		catch (UnsupportedMessage except) {
			// Cause 0x61 means "message type non-existant or not implemented".
			SDCCH->send(L3ChannelRelease(0x61));
		}

		// Wait for the channel to close.
		CLDCOUT("SDCCHDisptacher waiting for RELEASE");
		//FIXME -- What's the GSM 04.08 Txxx value for this?
		// Probably N200 * T200
		waitForPrimitive(SDCCH,RELEASE,20000);
	}
}



// vim: ts=4 sw=4
