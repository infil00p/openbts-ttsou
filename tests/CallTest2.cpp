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

/*
pthread thread numbers during testing:

1	main
2	rtp scheduler
3	SIP interface recv
4	clock interface
5	ARFCN 0 data interface
6	SCH generator
7	FCCH generator
8	BCCH generator
9	RACH recv
10	CCCH0
11	CCCH1
12	CCCH2
13	SDCCH0 L2 uplink
14	SACCH/S0 L2 generator
15	SACCH/S0 L2 uplink
16	SDCCH0 control thread
17	SDCCH1 L2 uplink
18	SACCH/S1 L2 generator
19	SACCH/S1 L2 uplink
20	SDCCH1 control thread
21	FACCH0 L2 uplink
22	SACCH/T1 L2 generator
23	TCH/FACCH0 L1 downlink
24	SACCH/T1 L2 uplink
25	FACCH0 control
26	FACCH0 L2 uplink
27	SACCH/T1 L2 generator
28	TCH/FACCH1 L1 downlink
29	SACCH/T1 L2 uplink
30	FACCH1 control
31	Pager
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

	// We deliberately configure two channel of
	// each type.
	// This is the minimum set to support phone-to-phone calling.
	// This makes channel state problems
	// more visible.

	// SDCCH 0
	SDCCHLogicalChannel SDCCH0(0,gSDCCH_4_0);
	Thread SDCCH0ControlThread;
	SDCCH0.downstream(radio);
	SDCCH0.open();
	gBTS.addSDCCH(&SDCCH0);
	SDCCH0ControlThread.start((void*(*)(void*))Control::SDCCHDispatcher,&SDCCH0);
	// SDCCH 1
	SDCCHLogicalChannel SDCCH1(0,gSDCCH_4_1);
	Thread SDCCH1ControlThread;
	SDCCH1.downstream(radio);
	SDCCH1.open();
	gBTS.addSDCCH(&SDCCH1);
	SDCCH1ControlThread.start((void*(*)(void*))Control::SDCCHDispatcher,&SDCCH1);

	// TCH 1
	TCHFACCHLogicalChannel TCH1(1,gTCHF_T1);
	Thread TCH1ControlThread;
	TCH1.downstream(radio);
	TCH1.open();
	gBTS.addTCH(&TCH1);
	TCH1ControlThread.start((void*(*)(void*))Control::FACCHDispatcher,&TCH1);
	// TCH 2
	TCHFACCHLogicalChannel TCH2(2,gTCHF_T2);
	Thread TCH2ControlThread;
	TCH2.downstream(radio);
	TCH2.open();
	gBTS.addTCH(&TCH2);
	TCH2ControlThread.start((void*(*)(void*))Control::FACCHDispatcher,&TCH2);

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
