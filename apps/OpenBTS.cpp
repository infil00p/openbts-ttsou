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



#include "TRXManager.h"
#include "GSML1FEC.h"
#include "GSMConfig.h"
#include "GSMSAPMux.h"
#include "GSML3RRMessages.h"
#include "GSMLogicalChannel.h"

#include "SIPInterface.h"
#include "Globals.h"

using namespace GSM;

// Load configuration from a file.
ConfigurationTable gConfig("OpenBTS.config");


// All of the other globals that rely on the global configuration file need to
// be declared here.

// The global SIPInterface object.
SIP::SIPInterface gSIPInterface;


// Configure the BTS object based on the config file.
GSMConfig gBTS(
	gConfig.getNum("GSM.NCC"),
	gConfig.getNum("GSM.BCC"),
	(GSMBand)gConfig.getNum("GSM.Band"),
	L3LocationAreaIdentity(
		gConfig.getStr("GSM.MCC"),
		gConfig.getStr("GSM.MNC"),
		gConfig.getNum("GSM.LAC")),
	L3CellIdentity(gConfig.getNum("GSM.CI")),
	gConfig.getStr("GSM.ShortName"));
// ARFCN is set with the ARFCNManager::tune method after the BTS is running.
const unsigned ARFCN=gConfig.getNum("GSM.ARFCN");

TransceiverManager gTRX(1, gConfig.getStr("TRX.IP"), gConfig.getNum("TRX.Port"));



int main(int argc, char *argv[])
{
	//srandomdev();

	CERR("INFO -- OpenBTS  Copyright (C) 2008, 2009 Free Software Foundation, Inc.");
	CERR("INFO -- This program comes with ABSOLUTELY NO WARRANTY;");
	CERR("INFO -- This is free software; you are welcome to redistribute it under the terms of GPLv3.");
	CERR("INFO -- Use of this software may be subject to other legal restrictions,");
	CERR("INFO -- including patent licsensing and radio spectrum licensing.");
	CERR("INFO -- All users of this software are expected to comply with applicable regulations");

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
       	radio->setPower(gConfig.getNum("GSM.PowerAttenDB"));

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


	// SDCCHs
	SDCCHLogicalChannel SDCCH[4] = { 
		SDCCHLogicalChannel(0,gSDCCH_4_0),
		SDCCHLogicalChannel(0,gSDCCH_4_1),
		SDCCHLogicalChannel(0,gSDCCH_4_2),
		SDCCHLogicalChannel(0,gSDCCH_4_3)
	};
	Thread SDCCHControlThread[4];
	for (int i=0; i<4; i++) {
		SDCCH[i].downstream(radio);
		SDCCHControlThread[i].start((void*(*)(void*))Control::DCCHDispatcher,&SDCCH[i]);
		SDCCH[i].open();
		gBTS.addSDCCH(&SDCCH[i]);
	}

	// TCHs
	TCHFACCHLogicalChannel TCH[7] = { 
		TCHFACCHLogicalChannel(1,gTCHF_T1),
		TCHFACCHLogicalChannel(2,gTCHF_T2),
		TCHFACCHLogicalChannel(3,gTCHF_T3),
		TCHFACCHLogicalChannel(4,gTCHF_T4),
		TCHFACCHLogicalChannel(5,gTCHF_T5),
		TCHFACCHLogicalChannel(6,gTCHF_T6),
		TCHFACCHLogicalChannel(7,gTCHF_T7)
	};
	Thread TCHControlThread[7];
	for (int i=0; i<7; i++) {
		TCH[i].downstream(radio);
		TCHControlThread[i].start((void*(*)(void*))Control::DCCHDispatcher,&TCH[i]);
		TCH[i].open();
		gBTS.addTCH(&TCH[i]);
	}

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
		sleep(20);
		CERR("NOTICE -- uptime " << gBTS.uptime() << " seconds, frame " << gBTS.time());
	}
}
