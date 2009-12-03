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



#include <TRXManager.h>
#include <GSML1FEC.h>
#include <GSMConfig.h>
#include <GSMSAPMux.h>
#include <GSML3RRMessages.h>
#include <GSMLogicalChannel.h>

#include <SIPInterface.h>
#include <Globals.h>

#include <Logger.h>
#include <CLI.h>

#include <unistd.h>
#include <string.h>
#include <signal.h>

using namespace std;
using namespace GSM;

// Load configuration from a file.
ConfigurationTable gConfig("OpenBTS.config");


// All of the other globals that rely on the global configuration file need to
// be declared here.

// The global SIPInterface object.
SIP::SIPInterface gSIPInterface;


// Configure the BTS object based on the config file.
// So don't create this until AFTER loading the config file.
GSMConfig gBTS;

// Our interface to the software-defined radio.
TransceiverManager gTRX(1, gConfig.getStr("TRX.IP"), gConfig.getNum("TRX.Port"));



int main(int argc, char *argv[])
{
	srandom(time(NULL));

	COUT("\n\n" << gOpenBTSWelcome << "\n");
	COUT("\nStarting the system...");

	gSetLogLevel(gConfig.getStr("LogLevel"));
	if (gConfig.defines("LogFileName")) {
		gSetLogFile(gConfig.getStr("LogFileName"));
	}

	// Start the transceiver binary, if the path is defined.
	// If the path is not defined, the transceiver must be started by some other process.
	const char *TRXPath = NULL;
	if (gConfig.defines("TRX.Path")) TRXPath=gConfig.getStr("TRX.Path");
	pid_t transceiverPid = 0;
	if (TRXPath) {
		const char *TRXLogLevel = gConfig.getStr("TRX.LogLevel");
		const char *TRXLogFileName = NULL;
		if (gConfig.defines("TRX.LogFileName")) TRXLogFileName=gConfig.getStr("TRX.LogFileName");
		transceiverPid = vfork();
		assert(transceiverPid>=0);
		if (transceiverPid==0) {
			execl(TRXPath,"transceiver",TRXLogLevel,TRXLogFileName,NULL);
			LOG(ERROR) << "cannot start transceiver";
			_exit(0);
		}
	}

	// Start the SIP interface.
	gSIPInterface.start();

	// Start the transceiver interface.
	gTRX.start();

	// Set up the interface to the radio.
	// Get a handle to the C0 transceiver interface.
	ARFCNManager* radio = gTRX.ARFCN(0);

	// Tuning.
	// Make sure its off for tuning.
	radio->powerOff();
	// Set TSC same as BSC everywhere.
	radio->setTSC(gBTS.BCC());
	// Tune.
      	radio->tune(gConfig.getNum("GSM.ARFCN"));
	// C-V on C0T0
	radio->setSlot(0,5);

	// Turn on and power up.
	radio->powerOn();
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

	// C-V C0T0 SDCCHs
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


	// Count configured slots.
	unsigned sCount = 1;

	// Create C-VII slots on C0Tn
	for (unsigned i=0; i<gConfig.getNum("GSM.NumC7s"); i++) {
		radio->setSlot(sCount,7);
		for (unsigned sub=0; sub<8; sub++) {
			SDCCHLogicalChannel* chan = new SDCCHLogicalChannel(sCount,gSDCCH8[sub]);
			chan->downstream(radio);
			Thread* thread = new Thread;
			thread->start((void*(*)(void*))Control::DCCHDispatcher,chan);
			chan->open();
			gBTS.addSDCCH(chan);
		}
		sCount++;
	}


	// Create C-I slots on C0Tn
	for (unsigned i=0; i<gConfig.getNum("GSM.NumC1s"); i++) {
		radio->setSlot(sCount,1);
		TCHFACCHLogicalChannel* chan = new TCHFACCHLogicalChannel(sCount,gTCHF_T[sCount]);
		chan->downstream(radio);
		Thread* thread = new Thread;
		thread->start((void*(*)(void*))Control::DCCHDispatcher,chan);
		chan->open();
		gBTS.addTCH(chan);
		sCount++;
	}

	assert(sCount<=8);



	/*
		Note: The number of different paging subchannels on       
		the CCCH is:                                        
                                                           
		MAX(1,(3 - BS-AG-BLKS-RES)) * BS-PA-MFRMS           
			if CCCH-CONF = "001"                        
		(9 - BS-AG-BLKS-RES) * BS-PA-MFRMS                  
			for other values of CCCH-CONF               
	*/

	// Set up the pager.
	// Set up paging channels.
	gBTS.addPCH(&CCCH2);
	// Start the paging generator
	// Don't start the pager until some PCHs exist!!
	gBTS.pager().start();

	LOG(INFO) << "system ready";
	COUT("\n\nWelcome to OpenBTS.  Type \"help\" to see available commands.");
        // FIXME: We want to catch control-d (emacs keybinding for exit())

	while (1) {
		char inbuf[1024];
		cout << "\nOpenBTS> ";
		cin.getline(inbuf,1024,'\n');
		if (strcmp(inbuf,"exit")==0) break;
		gParser.process(inbuf,cout,cin);
	}

	if (transceiverPid) kill(transceiverPid,SIGKILL);
}
