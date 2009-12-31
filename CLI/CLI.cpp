/*
* Copyright 2009 Free Software Foundation, Inc.
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
#include <iterator>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <config.h>
#include "CLI.h"
#include <Logger.h>
#include <Globals.h>

#include <GSMConfig.h>
#include <GSMLogicalChannel.h>
#include <ControlCommon.h>
#include <TRXManager.h>
#include <PowerManager.h>

using namespace std;
using namespace CommandLine;

#define SUCCESS 0
#define BAD_NUM_ARGS 1
#define BAD_VALUE 2
#define NOT_FOUND 3
#define TOO_MANY_ARGS 4
#define FAILURE 5

extern TransceiverManager gTRX;

/** Standard responses in the CLI, much mach erorrCode enum. */
static const char* standardResponses[] = {
	"success", // 0
	"wrong number of arguments", // 1
	"bad argument(s)", // 2
	"command not found", // 3
	"too many arguments for parser", // 4
	"command failed", // 5
};




int Parser::execute(char* line, ostream& os, istream& is) const
{
	// tokenize
	char *argv[mMaxArgs];
	int argc = 0;
	char **ap;
	// This is (almost) straight from the man page for strsep.
	for (ap=argv; (*ap=strsep(&line," ")) != NULL; ) {
		if (**ap != '\0') {
			if (++ap >= &argv[mMaxArgs]) break;
			else argc++;
		}
	}
	// Blank line?
	if (!argc) return SUCCESS;
	// Find the command.
	ParseTable::const_iterator cfp = mParseTable.find(argv[0]);
	if (cfp == mParseTable.end()) {
		return NOT_FOUND;
	}
	int (*func)(int,char**,ostream&,istream&);
	func = cfp->second;
	// Do it.
	int retVal = (*func)(argc,argv,os,is);
	// Give hint on bad # args.
	if (retVal==BAD_NUM_ARGS) os << help(argv[0]) << endl;
	return retVal;
}


void Parser::process(const char* line, ostream& os, istream& is) const
{
	char *newLine = strdup(line);
	int retVal = execute(newLine,os,is);
	free(newLine);
	if (retVal) os << standardResponses[retVal] << endl;
}


const char * Parser::help(const string& cmd) const
{
	HelpTable::const_iterator hp = mHelpTable.find(cmd);
	if (hp==mHelpTable.end()) return "no help available";
	return hp->second.c_str();
}



/**@name Commands for the CLI. */
//@{

/*
	A CLI command takes the argument in an array.
	It returns 0 on success.
*/

/** Get/set the logging level. */
int logLevel(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==1) {
		os << gGetLogLevelName() << endl;
		return SUCCESS;
	}
	if (argc!=2) return BAD_NUM_ARGS;
	if (gSetLogLevel(argv[1])) {
		gConfig.set("LogLevel",argv[1]);
		return SUCCESS;
	}
	return BAD_VALUE;
}

/** Set the logging file. */
int setLogFile(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=2) return BAD_NUM_ARGS;
	if (gSetLogFile(argv[1])) {
		gConfig.set("LogFile",argv[1]);
		return SUCCESS;
	}
	os << "cannot open " << argv[1] << " for logging" << endl;
	return FAILURE;
}


/** Display system uptime and current GSM frame number. */
int showUptime(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os.precision(2);
	os << "Unix time " << time(NULL) << endl;
	int seconds = gBTS.uptime();
	if (seconds<120) {
		os << "uptime " << seconds << " seconds, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float minutes = seconds / 60.0F;
	if (minutes<120) {
		os << "uptime " << minutes << " minutes, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float hours = minutes / 60.0F;
	if (hours<48) {
		os << "uptime " << hours << " hours, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float days = hours / 24.0F;
	os << "uptime " << days << " days, frame " << gBTS.time() << endl;
	return SUCCESS;
}


/** Give a list of available commands or describe a specific command. */
int getHelp(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==2) {
		os << argv[1] << " " << gParser.help(argv[1]) << endl;
		return SUCCESS;
	}
	if (argc!=1) return BAD_NUM_ARGS;
	ParseTable::const_iterator cp = gParser.begin();
	os << "Type \"help\" followed by the command name for help on that command." << endl;
	int c=0;
	while (cp != gParser.end()) {
		os << cp->first << '\t';
		++cp;
		c++;
		if (c%5==0) os << endl;
	}
	if (c%5!=0) os << endl;
	return SUCCESS;
}



/** A dummy function used to fill space in the parser table. */
int dummy(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=1) return BAD_NUM_ARGS;
	return SUCCESS;
}



/** Print or clear the TMSI table. */
int printTMSIs(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==2) {
		if (strcmp(argv[1],"clear")==0) {
			os << "clearing TMSI table" << endl;
			gTMSITable.clear();
			return SUCCESS;
		}
		return BAD_VALUE;
	}
	if (argc!=1) return BAD_NUM_ARGS;
	os << " IMSI             TMSI" << endl;
	Control::TMSIMap::const_iterator tp = gTMSITable.begin();
	while (tp != gTMSITable.end()) {
		os << tp->second << " 0x" << std::hex << tp->first << std::dec << endl;
		++tp;
	}
	os << endl << gTMSITable.size() << " TMSIs in table";
	return SUCCESS;
}

int findIMSI(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=2) {
		os << "usage: findimsi <imsiprefix>\n";
		return BAD_VALUE;
	}
	Control::TMSIMap::const_iterator tp = gTMSITable.begin();
	char buf[50]; // max size in decimal digits plus 1 plus RandomPositive
	while (tp != gTMSITable.end()) {
		std::string target;
		sprintf(buf, "%d", tp->first);
		std::string imsi = buf;
		target.assign(imsi, 0, strlen(argv[1]));
		if (target == argv[1])
			os << tp->second << " 0x" << std::hex << tp->first << std::dec << endl;
		++tp;
	}
	return SUCCESS;
}

int dumpTMSIs(int argc, char** argv, ostream& os, istream& is)
{
	if (argc != 2) {
		os << "usage: dumptmsis <filename>\n";
		return BAD_VALUE;
	}
	char* subargv[] = {"tmsis", NULL};
	int subargc = 1;
	ofstream fileout;
	fileout.open(argv[1], ios::out); // erases existing!
	printTMSIs(subargc, subargv, fileout, is);
	return SUCCESS;
}

/** Submit an SMS for delivery to an IMSI. */
int sendSMS(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=3) return BAD_NUM_ARGS;

	os << "enter text to send: ";
	char txtBuf[161];
	cin.getline(txtBuf,160,'\n');
	char *IMSI = argv[1];
	char *srcAddr = argv[2];

	Control::TransactionEntry transaction(
		GSM::L3MobileIdentity(IMSI),
		GSM::L3CMServiceType::MobileTerminatedShortMessage,
		GSM::L3CallingPartyBCDNumber(srcAddr),
		txtBuf);
	transaction.Q931State(Control::TransactionEntry::Paging);
	Control::initiateMTTransaction(transaction,GSM::SDCCHType,30000);
	os << "message submitted for delivery" << endl;
	return SUCCESS;
}

/** DEBUGGING: Sends a special sms that triggers a RRLP message to an IMSI. */
int sendRRLP(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=3) return BAD_NUM_ARGS;

	char *IMSI = argv[1];

	UDPSocket sock(0,"127.0.0.1",gConfig.getNum("SIP.Port"));
	unsigned port = sock.port();
	unsigned callID = random();

	// Just fake out a SIP message.
	const char form[] = "MESSAGE sip:IMSI%s@localhost SIP/2.0\nVia: SIP/2.0/TCP localhost;branch=z9hG4bK776sgdkse\nMax-Forwards: 2\nFrom: RRLP@localhost:%d;tag=49583\nTo: sip:IMSI%s@localhost\nCall-ID: %d@127.0.0.1:5063\nCSeq: 1 MESSAGE\nContent-Type: text/plain\nContent-Length: %lu\n\n%s\n";

	char txtBuf[161];
	sprintf(txtBuf,"RRLP%s",argv[2]);
	char outbuf[2048];
	sprintf(outbuf,form,IMSI,port,IMSI,callID,strlen(txtBuf),txtBuf);
	sock.write(outbuf);
	sleep(2);
	sock.write(outbuf);
	sock.close();
	os << "RRLP Triggering message submitted for delivery" << endl;

	return SUCCESS;
}



/** Print current usage loads. */
int printStats(int argc, char** argv, ostream& os, istream& is)
{
	os << "SDCCH load: " << gBTS.SDCCHActive() << '/' << gBTS.SDCCHTotal() << endl;
	os << "TCH/F load: " << gBTS.TCHActive() << '/' << gBTS.TCHTotal() << endl;
	os << "AGCH/PCH load: " << gBTS.AGCHLoad() << '/' << gBTS.PCHLoad() << endl;
	// paging table size
	os << "Paging table size: " << gBTS.pager().pagingEntryListSize() << endl;
	os << "Transactions/TMSIs: " << gTransactionTable.size() << "/" << gTMSITable.size() << endl;
	// 3122 timer current value (the number of seconds an MS should hold off the next RACH)
	os << "T3122: " << gBTS.T3122() << " ms" << endl;
	return SUCCESS;
}



/** Get/Set MCC, MNC, LAC, CI. */
int cellID(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==1) {
		os << "MCC=" << gConfig.getStr("GSM.MCC")
		<< " MNC=" << gConfig.getStr("GSM.MNC")
		<< " LAC=" << gConfig.getNum("GSM.LAC")
		<< " CI=" << gConfig.getNum("GSM.CI")
		<< endl;
		return SUCCESS;
	}

	if (argc!=5) return BAD_NUM_ARGS;

	// Safety check the args!!
	char* MCC = argv[1];
	char* MNC = argv[2];
	if (strlen(MCC)!=3) {
		os << "MCC must be three digits" << endl;
		return BAD_VALUE;
	}
	int MNCLen = strlen(MNC);
	if ((MNCLen<2)||(MNCLen>3)) {
		os << "MNC must be two or three digits" << endl;
		return BAD_VALUE;
	}
	gTMSITable.clear();
	gConfig.set("GSM.MCC",MCC);
	gConfig.set("GSM.MNC",MNC);
	gConfig.set("GSM.LAC",argv[3]);
	gConfig.set("GSM.CI",argv[4]);
	gBTS.regenerateBeacon();
	return SUCCESS;
}



/** Get/Set TCH assignment type in call setup. */
int assignmentType(int argc, char** argv, ostream& os, istream& is)
{
	// These must match AssignmentType emun in GSMCommon.h.
	static const char* names[] = {"early", "veryearly"};
	static const char* nums[] = {"0","1"};
	if (argc==1) {
		os << names[gConfig.getNum("GSM.AssignmentType")] << endl;
		return SUCCESS;
	}
	if (argc!=2) return BAD_NUM_ARGS;
	for (int i=0; i<2; i++) {
		if (strcmp(argv[1],names[i])==0) {
			gConfig.set("GSM.AssignmentType",nums[i]);
			return SUCCESS;
		}
	}
	os << "\"" << argv[1] << "\" is not a valid assignment type" << endl;
	return BAD_VALUE;
}



/** Print table of current transactions. */
int printTransactions(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=1) return BAD_NUM_ARGS;
	Control::TransactionMap::const_iterator trans = gTransactionTable.begin();
	int count = 0;
	gTransactionTable.clearDeadEntries();
	while (trans != gTransactionTable.end()) {
		os << trans->second << endl;
		++trans;
		count++;
	}
	os << endl << count << " transactions in table" << endl;
	return SUCCESS;
}



/** Print current configuration table. */
int config(int argc, char** argv, ostream& os, istream& is)
{
	// no args, just print
	if (argc==1) {
		gConfig.dump(os);
		return SUCCESS;
	}

	// one arg, pattern match
	if (argc==2) {
		StringMap::const_iterator p = gConfig.begin();
		while (p != gConfig.end()) {
			if (strstr(p->first.c_str(),argv[1]))
				os << p->first << ": " << p->second << endl;
			++p;
		}
		return SUCCESS;
	}

	// >1 args: set new value
	string val;
	for (int i=2; i<argc; i++) val.append(argv[i]);
	if (!gConfig.set(argv[1],val)) {
		os << argv[1] << " is static and connot be altered after initialization" << endl;
		return BAD_VALUE;
	}
	gBTS.regenerateBeacon();
	return SUCCESS;
}


/** Dump current configuration to a file. */
int dumpConfig(int argc, char** argv, ostream& os, istream& is)
{
	if (argc!=2) return BAD_NUM_ARGS;
	fstream f;
	f.open(argv[1],fstream::out);
	if (f.fail()) {
		os << "cannot open " << argv[1] << " for writing" << endl;
		return FAILURE;
	}
	f << "# OpenBTS configuration file" << endl;
	time_t now = time(NULL);
	f << "# " << ctime(&now) << endl;
	gConfig.dump(f);
	f.close();
	return SUCCESS;
}



/** Change the registration timers. */
int configRegistration(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==1) {
		os << "T3212 is " << gConfig.getNum("GSM.T3212") << " minutes" << endl;
		os << "SIP registration period is " << gConfig.getNum("SIP.RegistrationPeriod")/60 << " minutes" << endl;
		return SUCCESS;
	}

	if (argc>3) return BAD_NUM_ARGS;

	unsigned newT3212 = strtol(argv[1],NULL,10);
	if ((newT3212<6)||(newT3212>1530)) {
		os << "valid T3212 range is 6..1530 minutes" << endl;
		return BAD_VALUE;
	}

	// By defuault, make SIP registration period 1.5x the GSM registration period.
	unsigned SIPRegPeriod = newT3212*90;
	if (argc==3) {
		SIPRegPeriod = 60*strtol(argv[2],NULL,10);
	}

	// Set the values in the table and on the GSM beacon.
	gConfig.set("SIP.RegistrationPeriod",SIPRegPeriod);
	gConfig.set("GSM.T3212",newT3212);
	gBTS.regenerateBeacon();
	// Done.
	return SUCCESS;
}


/** Get/set the network short name. */
int shortName(int argc, char** argv, ostream& os, istream& is)
{
	if (argc==1) {
		os << gConfig.getStr("GSM.ShortName") << endl;
		return SUCCESS;
	}

	if (strlen(argv[1])>7) {
		os << "short name must be less than 8 characters" << endl;
		return BAD_VALUE;
	}

	gConfig.set("GSM.ShortName",argv[1]);
	return SUCCESS;
}

/** Print the list of alarms kept by the logger, i.e. the last LOG(ALARM) << <text> */
int printAlarms(int argc, char** argv, ostream& os, istream& is)
{
	std::ostream_iterator<std::string> output( os, "\n" );
	std::list<std::string> alarms = gGetLoggerAlarms();
	std::copy( alarms.begin(), alarms.end(), output );
	return SUCCESS;
}


/** Version string. */
int version(int argc, char **argv, ostream& os, istream& is)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os << VERSION << endl;
	return SUCCESS;
}

int page(int argc, char **argv, ostream& os, istream& is)
{
	if (argc==1) {
		gBTS.pager().dump(os);
		return SUCCESS;
	}
	if (argc!=3) return BAD_NUM_ARGS;
	char *IMSI = argv[1];
	if (strlen(IMSI)>15) {
		os << IMSI << " is not a valid IMSI" << endl;
		return BAD_VALUE;
	}
	Control::TransactionEntry dummy;
	gBTS.pager().addID(GSM::L3MobileIdentity(IMSI),GSM::SDCCHType,dummy,1000*atoi(argv[2]));
	return SUCCESS;
}



int testcall(int argc, char **argv, ostream& os, istream& is)
{
	if (argc!=3) return BAD_NUM_ARGS;
	char *IMSI = argv[1];
	if (strlen(IMSI)!=15) {
		os << IMSI << " is not a valid IMSI" << endl;
		return BAD_VALUE;
	}
	Control::TransactionEntry transaction(
		GSM::L3MobileIdentity(IMSI),
		GSM::L3CMServiceType::TestCall,
		GSM::L3CallingPartyBCDNumber("0"));
	transaction.Q931State(Control::TransactionEntry::Paging);
	Control::initiateMTTransaction(transaction,GSM::TCHFType,1000*atoi(argv[2]));
	return SUCCESS;
}


int endcall(int argc, char **argv, ostream& os, istream& is)
{
	if (argc!=2) return BAD_NUM_ARGS;
	unsigned transID = atoi(argv[1]);
	Control::TransactionEntry target;
	if (!gTransactionTable.find(transID,target)) {
		os << transID << " not found in table";
		return BAD_VALUE;
	}
	target.Q931State(Control::TransactionEntry::ReleaseRequest);
	gTransactionTable.update(target);
	return SUCCESS;
}


int rolllac(int argc, char **argv, ostream& os, istream& is)
{
	int newLAC;
	if (argc==1) newLAC = gConfig.getNum("GSM.LAC") + 1;
	else if (argc==2) newLAC = atoi(argv[1]);
	else return BAD_NUM_ARGS;

	gConfig.set("GSM.LAC",newLAC);
	cellID(1,argv,os,is);
	gBTS.regenerateBeacon();
	gTMSITable.clear();
	return SUCCESS;
}





void printChanInfo(const GSM::LogicalChannel* chan, ostream& os)
{
		os << chan->TN() << " " << chan->typeAndOffset() << " ";
		os << (int)round(chan->FER()*100) << " ";
		os << (int)round(chan->RSSI()) << " ";
		os << chan->actualMSPower() << " " << chan->actualMSTiming() << " ";
		const GSM::L3MeasurementResults& meas = chan->SACCH()->measurementResults();
		if (meas.MEAS_VALID()) {
			os << meas.RXLEV_FULL_SERVING_CELL() << " ";
			os << meas.RXQUAL_FULL_SERVING_CELL() << " ";
		}
		os << endl;
}



int chans(int argc, char **argv, ostream& os, istream& is)
{
	if (argc!=1) return BAD_NUM_ARGS;

	os << "TN chan FER RSSI TXPWR TXTA RXLEV RXQUAL" << endl;
	os << "TN type \%   dB   dBm   sym " << endl;

	// TCHs
	GSM::TCHList::const_iterator tChanItr = gBTS.TCHPool().begin();
	while (tChanItr != gBTS.TCHPool().end()) {
		const GSM::TCHFACCHLogicalChannel* tChan = *tChanItr;
		if (tChan->active()) printChanInfo(tChan,os);
		++tChanItr;
	}

	// SDCCHs
	GSM::SDCCHList::const_iterator sChanItr = gBTS.SDCCHPool().begin();
	while (sChanItr != gBTS.SDCCHPool().end()) {
		const GSM::SDCCHLogicalChannel* sChan = *sChanItr;
		if (sChan->active()) printChanInfo(sChan,os);
		++sChanItr;
	}


	return SUCCESS;
}




int power(int argc, char **argv, ostream& os, istream& is)
{
	os << "current downlink power " << gBTS.powerManager().power() << " dB wrt full scale" << endl;
	os << "current attenuation bounds "
		<< gConfig.getNum("GSM.PowerManager.MinAttenDB")
		<< " to "
		<< gConfig.getNum("GSM.PowerManager.MaxAttenDB")
		<< " dB" << endl;

	if (argc==1) return SUCCESS;
	if (argc!=3) return BAD_NUM_ARGS;

	int min = atoi(argv[1]);
	int max = atoi(argv[2]);
	if (min>max) return BAD_VALUE;

	gConfig.set("GSM.PowerManager.MinAttenDB",argv[1]);
	gConfig.set("GSM.PowerManager.MaxAttenDB",argv[2]);

	os << "new attenuation bounds "
		<< gConfig.getNum("GSM.PowerManager.MinAttenDB")
		<< " to "
		<< gConfig.getNum("GSM.PowerManager.MaxAttenDB")
		<< " dB" << endl;

	return SUCCESS;
}




//@} // CLI commands



Parser::Parser()
{
	// The constructor adds the commands.
	addCommand("loglevel", logLevel, "[level] -- get/set the logging level, one of {ERROR, ALARM, WARN, NOICE, INFO, DEBUG, DEEPDEBUG}.");
	addCommand("setlogfile", setLogFile, "<path> -- set the logging file to <path>.");
	addCommand("uptime", showUptime, "-- show BTS uptime and BTS frame number.");
	addCommand("help", getHelp, "[command] -- list available commands or gets help on a specific command.");
	addCommand("exit", dummy, "-- exit the application.");
	addCommand("tmsis", printTMSIs, "[\"clear\"] -- print/clear the TMSI table.");
	addCommand("findimsi", findIMSI, "-- [IMSIPrefix] - prints all imsi's that are prefixed by IMSIPrefix");
	addCommand("dumptmsis", dumpTMSIs, "-- dump TMSI table to ");
	addCommand("sendsms", sendSMS, "<IMSI> <src> -- send SMS to <IMSI>, addressed from <src>, after prompting.");
	addCommand("sendrrlp", sendRRLP, "<IMSI> <hexstring> -- send RRLP message <hexstring> to <IMSI>.");
	addCommand("load", printStats, "-- print the current activity loads.");
	addCommand("cellid", cellID, "[MCC MNC LAC CI] -- get/set location area identity (MCC, MNC, LAC) and cell ID (CI)");
	addCommand("assignment", assignmentType, "[type] -- get/set assignment type (early, veryearly)");
	addCommand("calls", printTransactions, "-- print the transaction table");
	addCommand("config", config, "[] OR [patt] OR [key val(s)] -- print the current configuration, print configuration values matching a pattern, or set/change a configuration value");
	addCommand("configsave", dumpConfig, "<path> -- write the current configuration to a file");
	addCommand("regperiod", configRegistration, "[GSM] [SIP] -- get/set the registration period (GSM T3212), in MINUTES");
	addCommand("shortname", shortName, "[name] -- get/set the network short name");
	addCommand("alarms", printAlarms, "-- show latest alarms");
	addCommand("version", version,"-- print the version string");
	addCommand("page", page, "IMSI time -- page the given IMSI for the given period");
	addCommand("testcall", testcall, "IMSI time -- initiate a test call to a given IMSI with a given paging time");
	addCommand("endcall", endcall,"trans# -- terminate the given transaction");
	addCommand("rolllac", rolllac, "[LAC] -- increment the LAC or set a net value");
	addCommand("chans", chans, "-- report PHY status for active channels");
	addCommand("power", power, "[minAtten maxAtten] -- report current attentuation or set min/max bounds");

	// TODO -- Commands to add: FER, CI.
}




// vim: ts=4 sw=4
