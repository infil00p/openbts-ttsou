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



#include "TRXManager.h"
#include "GSML1FEC.h"
#include "GSMConfig.h"
#include "GSMSAPMux.h"
#include "GSML3RRMessages.h"
#include "GSMLogicalChannel.h"

#include "SIPInterface.h"

using namespace GSM;


// Example BTS configuration:
// 0 -- network color code
// 0 -- basestation color code
// EGSM900 -- the operating band
// LAI: 901 -- mobile country code
// LAI: 55 -- mobile network code
// LAI: 667 -- location area code ("the neighbors of the beast")
// WASTE -- network "short name", displayed on nicer/newer phones
//	("WASTE" is a reference from Thomas Pynchon's _The_Crying_of_Lot_49_, BTW.)
GSMConfig gBTS(0,0,EGSM900,L3LocationAreaIdentity("901","55",1234),L3CellIdentity(0x0),"WASTE");
// ARFCN is set with the ARFCNManager::tune method after the BTS is running.
const unsigned ARFCN=29;

TransceiverManager gTRX(1, "127.0.0.1", 5700);

// Set interface with local ip and port=SIP_UDP_PORT. Asterisk port=5060
SIP::SIPInterface gSIPInterface(SIP_UDP_PORT, "127.0.0.1", 5060);





int main(int argc, char *argv[])
{
	gSIPInterface.start();
	gTRX.start();

	// Set up the interface to the radio.
	ARFCNManager* radio = gTRX.ARFCN(0);
	// Get a handle to the C0 transceiver interface.
      	radio->tune(ARFCN);
	// C-V on C0T0
	radio->setSlot(0,5);
	// C-I on C0T1-C0T7
	for (unsigned i=1; i<8; i++) radio->setSlot(i,1);
       	radio->setTSC(gBTS.BCC());
       	radio->setPower(0);

	// set up a combination V beacon set

	// SCH
	SCHL1FEC SCH;
	SCH.downstream(radio);
	SCH.open();
	// FCCH
	FCCHL1FEC FCCH;
	FCCH.downstream(radio);
	FCCH.open();
	// BCCH
	BCCHL1FEC BCCH;
	BCCH.downstream(radio);
	BCCH.open();
	// RACH
	RACHL1FEC RACH(gRACHC5Mapping);
	RACH.downstream(radio);
	RACH.open();
	// CCCHs
	CCCHLogicalChannel CCCH0(gCCCH_0Mapping);
	CCCH0.downstream(radio);
	CCCH0.open();
	CCCHLogicalChannel CCCH1(gCCCH_1Mapping);
	CCCH1.downstream(radio);
	CCCH1.open();
	CCCHLogicalChannel CCCH2(gCCCH_2Mapping);
	CCCH2.downstream(radio);
	CCCH2.open();
	// use CCCHs as AGCHs
	gBTS.addAGCH(&CCCH0);
	gBTS.addAGCH(&CCCH1);
	gBTS.addAGCH(&CCCH2);

	// We deliberately configure one channel of
	// each type.  This makes channel state problems
	// more visible.

	// SDCCH
	SDCCHLogicalChannel SDCCH(0,gSDCCH_4_0);
	Thread SDCCHControlThread;
	SDCCH.downstream(radio);
	SDCCH.open();
	gBTS.addSDCCH(&SDCCH);
	SDCCHControlThread.start((void*(*)(void*))Control::SDCCHDispatcher,&SDCCH);

	// TCH
	TCHFACCHLogicalChannel TCH(1,gTCHF_T1);
	Thread TCHControlThread;
	TCH.downstream(radio);
	TCH.open();
	gBTS.addTCH(&TCH);
	TCHControlThread.start((void*(*)(void*))Control::FACCHDispatcher,&TCH);

	// Set up the pager.
	// Set up paging channels.
	gBTS.addPCH(&CCCH0);
	gBTS.addPCH(&CCCH1);
	gBTS.addPCH(&CCCH2);
	// Start the paging generator
	// Don't start the pager until some PCHs exist!!
	gBTS.pager().start();

	// Just sleep now.
	while (1) {
		sleep(1);
	}
}