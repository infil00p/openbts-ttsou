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

#include <string.h>
#include <cstdio>
#include <fstream>

#include "Configuration.h"
#include "Sockets.h"
#include "Logger.h"
#include "Timeval.h"


using namespace std;


/** Names of the logging levels. */
static const char* levelNames[] =
	{ "FORCE", "ERROR", "ALARM", "WARNING", "NOTICE", "INFO", "DEBUG", "DEEPDEBUG" };

/** The global logging lock. */
static Mutex gLogLock;

/**@ The global alarms table. */
//@{
const unsigned  gMaxAlarms = 10;
Mutex           gAlarmsLock;
list<string>    gAlarmsList;
void            gAddAlarm(const string&);
const unsigned int DEFAULT_ALARM_TARGET_PORT = 10101;
const char*     DEFAULT_ALARM_TARGET_IP = "127.0.0.1";
unsigned int    gAlarmTargetPort = DEFAULT_ALARM_TARGET_PORT;
std::string     gAlarmTargetIP = DEFAULT_ALARM_TARGET_IP;
//@}


/** The current global logging level. */
static Log::Level gLoggingLevel = Log::LOG_WARN;




Log::Level gGetLogLevel()
{
	return gLoggingLevel;
}

const char* gGetLogLevelName()
{
	return levelNames[gLoggingLevel];
}

void gSetLogLevel(Log::Level level)
{
	gLoggingLevel = level;
}

bool gSetLogLevel(const char* name)
{
	for (int i=0; i<8; i++) {
		if (strcmp(name,levelNames[i])==0) {
			gLoggingLevel = (Log::Level)i;
			return true;
		}
	}
	LOG(ERROR) << '\"' << name << '\"' << " is not a valid logging level.";
	return false;
}


/** The current global log sink. */
static FILE *gLoggingFile = stdout;

void gSetLogFile(FILE *wFile)
{
	gLogLock.lock();
	gLoggingFile = wFile;
	gLogLock.unlock();
}


bool gSetLogFile(const char *name)
{
	assert(name);
	LOG(FORCE) << "setting log path to " << name;
	bool retVal = true;
	gLogLock.lock();
	FILE* newLoggingFile = fopen(name,"a+");
	if (!newLoggingFile) {
		LOG(ERROR) << "cannot open \"" << name << "\" for logging.";
		retVal = false;
	} else {
		gLoggingFile = newLoggingFile;
	}
	gLogLock.unlock();
	LOG(FORCE) << "new log path " << name;
	return retVal;
}

void gSetAlarmTargetPort(unsigned int port)
{
	LOG(INFO) << "setting ALARM target port to " << port;
    gAlarmTargetPort = port;
}

void gSetAlarmTargetIP(const char* ip)
{
	LOG(INFO) << "setting ALARM target IP to " << ip;
    gAlarmTargetIP = ip;
}

// copies the alarm list and returns it. list supposed to be small.
std::list<std::string> gGetLoggerAlarms()
{
    gAlarmsLock.lock();
    std::list<std::string> ret;
    // excuse the "complexity", but to use std::copy with a list you need
    // an insert_iterator - copy technically overwrites, doesn't insert.
    std::insert_iterator< std::list<std::string> > ii(ret, ret.begin());
    std::copy(gAlarmsList.begin(), gAlarmsList.end(), ii);
    gAlarmsLock.unlock();
    return ret;
}

ostream& operator<<(ostream& os, Log::Level level)
{
	os << levelNames[(int)level];
	return os;
}


// Add an alarm to the alarm list, and send it out via udp
//
// On the first call we read the ip and port from the configuration
// TODO - is there any global setup function where this should be done? -- Alon
void gAddAlarm(const string& s)
{
    gAlarmsLock.lock();
	// Socket open and close on every alarm - wise?
	// Probably.  That way we are sure to pick up changes in the target address.
	// Alarms should not happen often.
	UDPSocket alarmsocket(0, gAlarmTargetIP.c_str(), gAlarmTargetPort);
	alarmsocket.write(s.c_str());
    // append to list and reduce list to gMaxAlarms
    gAlarmsList.push_back(s);
    while (gAlarmsList.size() > gMaxAlarms) gAlarmsList.pop_front();
    gAlarmsLock.unlock();
}


Log::~Log()
{
	gLogLock.lock();
	// XXX always handle alarms, even if the logging level is too low
	if (mReportLevel == LOG_ALARM) gAddAlarm(mStream.str().c_str());
	if (mReportLevel>gLoggingLevel) return;
	mStream << std::endl;
	fprintf(gLoggingFile, "%s", mStream.str().c_str());
	fflush(gLoggingFile);
	gLogLock.unlock();
}


ostringstream& Log::get()
{
	Timeval now;
	mStream.precision(4);
	mStream << now << ' ' << mReportLevel <<  ' ';
	return mStream;
}






// vim: ts=4 sw=4
