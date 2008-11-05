/**@file Idle-mode dispatcher for the FACCH. */

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




#include "ControlCommon.h"
#include "GSMLogicalChannel.h"
#include "GSML3RRMessages.h"

#ifndef LOCAL
#include "SIPUtility.h"
#include "SIPInterface.h"
#endif

#define DEBUG 1

using namespace std;
using namespace GSM;
using namespace Control;





/** Example of a closed-loop, persistent-thread control function for the FACCH. */
void Control::FACCHDispatcher(TCHFACCHLogicalChannel *TCH)
{
	while (1) {

		try {
			// Wait for a transaction to start.
			CLDCOUT("FACCHDispatcher waiting for ESTABLISH");
			while (!waitForPrimitive(TCH,ESTABLISH)) {}

			// First message should be the assignment confirm message.
			CLDCOUT("FACCHDispatcher waiting for assignment confirm");
			const L3AssignmentComplete* confirm = dynamic_cast<const L3AssignmentComplete*>(getMessage(TCH));
			if (confirm==NULL) throw UnexpectedMessage();
			delete confirm;

			// Check the transaction table to know what to do next.
			TransactionEntry transaction;
			if (!gTransactionTable.find(TCH->transactionID(),transaction)) {
				CLDCOUT("NOTICE -- Assignment Complete from channel with no transaction");
				throw UnexpectedMessage();
			}
			CLDCOUT("FACCHDispatcher AssignmentComplete service="<<transaction.service().type());
			// These "controller" functions don't return until the call is cleared.
			switch (transaction.service().type()) {
				case L3CMServiceType::MobileOriginatedCall:
					MOCController(transaction,TCH);
					break;
				case L3CMServiceType::MobileTerminatedCall:
					MTCController(transaction,TCH);
					break;
				default:
				CLDCOUT("NOTICE -- request for unsupported service " << transaction.service());
				throw UnsupportedMessage();
			}
			// If we got here, the call is cleared.
			clearTransactionHistory(TCH->transactionID());
		}


		catch (ChannelReadTimeout except) {
			clearTransactionHistory(except.transactionID());
			CERR("NOTICE -- ChannelReadTimeout");
			// Cause 0x03 means "abnormal release, timer expired".
			TCH->send(L3ChannelRelease(0x03));
		}
		catch (UnexpectedPrimitive except) {
			clearTransactionHistory(except.transactionID());
			CERR("NOTICE -- UnexpectedPrimitive");
			// Cause 0x62 means "message type not not compatible with protocol state".
			TCH->send(L3ChannelRelease(0x62));
		}
		catch (UnexpectedMessage except) {
			clearTransactionHistory(except.transactionID());
			CERR("NOTICE -- UnexpectedMessage");
			// Cause 0x62 means "message type not not compatible with protocol state".
			TCH->send(L3ChannelRelease(0x62));
		}
		catch (UnsupportedMessage except) {
			clearTransactionHistory(except.transactionID());
			CERR("NOTICE -- UnsupportedMessage");
			// Cause 0x61 means "message type not implemented".
			TCH->send(L3ChannelRelease(0x61));
		}
		catch (Q931TimerExpired except) {
			clearTransactionHistory(except.transactionID());
			CERR("NOTICE -- Q.931 T3xx timer expired");
			// Cause 0x03 means "abnormal release, timer expired".
			TCH->send(L3ChannelRelease(0x03));
		}
#ifndef LOCAL
		catch (SIP::SIPTimeout) {
			CERR("NOTICE -- Uncaught SIP Timeout");
			// Cause 0x03 means "abnormal release, timer expired".
			TCH->send(L3ChannelRelease(0x03));
		}
		catch (SIP::SIPError) {
			CERR("NOTICE -- Uncaught SIP Error");
			// Cause 0x01 means "abnormal release, unspecified".
			TCH->send(L3ChannelRelease(0x01));
		}
#endif

		// FIXME -- What's the GSM 04.08 Txxx value for this?
		// Probably T200 * N200.
		waitForPrimitive(TCH,RELEASE,20000);
	}
}


// vim: ts=4 sw=4
