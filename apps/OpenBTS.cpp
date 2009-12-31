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

#include <iostream>
#include <fstream>

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
#include <PowerManager.h>
#include <RRLPQueryController.h>

#include <assert.h>
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





bool select_on_stdin(unsigned timeout_secs)
{
	struct timeval       timeout;
	fd_set        working_set;
	int rc;
	int max_sd = 0;

	FD_ZERO(&working_set);
	max_sd = 0; // stdin
	FD_SET(0, &working_set);
	// wait for 10 seconds
	timeout.tv_sec  = timeout_secs;
	timeout.tv_usec = 0;
	rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);
	if (rc == 0) // timeout
	      return false;
	return true;
}

pid_t gTransceiverPid = 0;

void restartTransceiver()
{
	// This is harmless - if someone is running OpenBTS they WANT no transceiver
	// instance at the start anyway.
	if (gTransceiverPid > 0) {
		LOG(INFO) << "RESTARTING TRANSCEIVER";
		kill(gTransceiverPid,SIGKILL); // TODO - call on ctrl-c (put in signal?)
	}

	// Start the transceiver binary, if the path is defined.
	// If the path is not defined, the transceiver must be started by some other process.
	const char *TRXPath = NULL;
	if (gConfig.defines("TRX.Path")) TRXPath=gConfig.getStr("TRX.Path");
	if (TRXPath) {
		const char *TRXLogLevel = gConfig.getStr("TRX.LogLevel");
		const char *TRXLogFileName = NULL;
		if (gConfig.defines("TRX.LogFileName")) TRXLogFileName=gConfig.getStr("TRX.LogFileName");
		gTransceiverPid = vfork();
		assert(gTransceiverPid>=0);
		if (gTransceiverPid==0) {
			// Pid==0 means this is the process that starts the transceiver.
			execl(TRXPath,"transceiver",TRXLogLevel,TRXLogFileName,NULL);
			LOG(ERROR) << "cannot start transceiver";
			_exit(0);
		}
	}
}



class TransceiverHangupDetector {
public:
	TransceiverHangupDetector()
		: mLastUp()
	{
	}
	Timeval mLastUp;
	bool restartIfHung();
};

/**
  check if SDCCHTotal is 0, if so for more the TRX.HangupTimeout then restart
  transceiver.

  Return: true if restarted, false otherwise.
 */
bool TransceiverHangupDetector::restartIfHung()
{
	unsigned totalactive = gBTS.SDCCHActive() + gBTS.TCHActive();
	if (totalactive > 0) {
		mLastUp = Timeval(); // reset to now
		return false;
	}
	if (mLastUp.elapsed() >= 1000*gConfig.getNum("TRX.HangupTimeout")) {
		LOG(ALARM) << "transceiver hung, restarting";
		restartTransceiver();
		mLastUp = Timeval();
		return true;
	}
	return false;
}

TransceiverHangupDetector gHangup;

void logload(ostream& os)
{
	os << "SDCCH load," << gBTS.SDCCHActive() << ',' << gBTS.SDCCHTotal()
	<< ",TCH/F load," << gBTS.TCHActive() << ',' << gBTS.TCHTotal()
	<< ",AGCH/PCH load," << gBTS.AGCHLoad() << ',' << gBTS.PCHLoad()
	<< ",paging size," << gBTS.pager().pagingEntryListSize()
	<< ",T3122," << gBTS.T3122()
	<< ",transactions," << gTransactionTable.size()
	<< ",tmsis," << gTMSITable.size()
	<< ",RRLP," << RRLP::RRLPQueryManager::instance()->numQueries()
	<< ",GPS," << RRLP::RRLPQueryManager::instance()->numPositions()
	<< ",Hangup," << gHangup.mLastUp.elapsed()
	<< endl;
}

int main(int argc, char *argv[])
{
	srandom(time(NULL));

	COUT("\n\n" << gOpenBTSWelcome << "\n");
	COUT("\nStarting the system...");

	gSetLogLevel(gConfig.getStr("LogLevel"));
	if (gConfig.defines("LogFileName")) {
		gSetLogFile(gConfig.getStr("LogFileName"));
	}
	gSetAlarmTargetPort(gConfig.getNum("Alarm.TargetPort"));
	gSetAlarmTargetIP(gConfig.getStr("Alarm.TargetIP"));

	restartTransceiver();

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
	// Set TSC same as BCC everywhere.
	radio->setTSC(gBTS.BCC());
	// Tune.
	radio->tune(gConfig.getNum("GSM.ARFCN"));

	// Turn on and power up.
	radio->powerOn();
	radio->setPower(gConfig.getNum("GSM.PowerManager.MinAttenDB"));

        // Set maximum network delay
        radio->setMaxDelay(gConfig.getNum("GSM.MaxExpectedDelaySpread"));

	// C-V on C0T0
	radio->setSlot(0,5);
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
	SDCCHLogicalChannel C0T0SDCCH[4] = {
		SDCCHLogicalChannel(0,gSDCCH_4_0),
		SDCCHLogicalChannel(0,gSDCCH_4_1),
		SDCCHLogicalChannel(0,gSDCCH_4_2),
		SDCCHLogicalChannel(0,gSDCCH_4_3),
	};
	Thread C0T0SDCCHControlThread[4];
	for (int i=0; i<4; i++) {
		C0T0SDCCH[i].downstream(radio);
		C0T0SDCCHControlThread[i].start((void*(*)(void*))Control::DCCHDispatcher,&C0T0SDCCH[i]);
		C0T0SDCCH[i].open();
		gBTS.addSDCCH(&C0T0SDCCH[i]);
	}

	// Count configured slots.
	unsigned sCount = 1;

	// Create C-VII slots on C0Tn
	for (int i=0; i<gConfig.getNum("GSM.NumC7s"); i++) {
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
	for (int i=0; i<gConfig.getNum("GSM.NumC1s"); i++) {
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
	// HACK -- For now, use a single paging channel, since paging groups are broken.
	gBTS.addPCH(&CCCH2);

	// OK, now it is safe to start the BTS.
	gBTS.start();

	LOG(INFO) << "system ready";
	COUT("\n\nWelcome to OpenBTS.  Type \"help\" to see available commands.");
        // FIXME: We want to catch control-d (emacs keybinding for exit())

	ofstream logout;
	logout.open("log.out", ios::app | ios::out);
	Timeval last;
	cout << "\nOpenBTS> ";
	cout.flush();
	long timeout_millisecs = 5000;
	if (gConfig.defines("Main.SelectTimeout")) {
		timeout_millisecs = gConfig.getNum("Main.SelectTimeout");
	}
	long timeout_secs = timeout_millisecs / 1000;
	bool use_select = gConfig.defines("Main.Select") && gConfig.getNum("Main.Select") == 1;
	

	// 2 Purpose stuff:
	//  command line interface (gParser, CLI)
	//  load recording (this file)
	while (1) {
		if (!use_select || select_on_stdin(timeout_secs)) {
			char inbuf[1024];
			cin.getline(inbuf,1024,'\n');
			if (strcmp(inbuf,"exit")==0) break;
			gParser.process(inbuf,cout,cin);
			cout << "\nOpenBTS> ";
			cout.flush();
		}
		if (last.elapsed() >= timeout_millisecs) {
			last = Timeval();
			// log load
			//std::cout << "logging at " << last << "\n";
			logout << "Time," << last << ",Elapsed," << gBTS.uptime() << ",";
			logload(logout);
			//gParser.process("power", logout, cin);
			//gParser.process("load", logout, cin);
		}
		gHangup.restartIfHung();
	}

	if (gTransceiverPid) kill(gTransceiverPid,SIGKILL);
}

