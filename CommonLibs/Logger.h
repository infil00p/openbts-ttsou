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


#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include "Threads.h"



#define _LOG(level) Log(Log::LOG_##level).get() << pthread_self() << ' ' << __FILE__ << ':' << __LINE__  << ':' << __FUNCTION__ << ": "
#define LOG(wLevel) \
	if (Log::LOG_##wLevel > gGetLogLevel()) ; \
	else _LOG(wLevel)
#define OBJLOG(wLevel) \
	if (Log::LOG_##wLevel > gGetLogLevel()) ; \
	else _LOG(wLevel) << "obj:" << this << ' '




/**
	A thread-safe logger, directable to any file or stream.
	Derived from Dr. Dobb's Sept. 2007 issue.
*/
class Log {

	public:

	/** Available logging levels. */
	enum Level {
		LOG_FORCE,
		LOG_ERROR,
		LOG_ALARM,
		LOG_WARN,
		LOG_NOTICE,
		LOG_INFO,
		LOG_DEBUG,
		LOG_DEEPDEBUG
	};

	protected:

	std::ostringstream mStream;	///< This is where we write the long.
	Level mReportLevel;			///< Level of current repot.

	static FILE *sFile;


	public:

	Log(Level wReportLevel = LOG_WARN)
		:mReportLevel(wReportLevel)
	{ }

	~Log();

	std::ostringstream& get();
};

std::ostringstream& operator<<(std::ostringstream& os, Log::Level);


/**@ Global logging level control. */
//@{
void gSetLogLevel(Log::Level);
bool gSetLogLevel(const char*);
Log::Level gGetLogLevel();
const char* gGetLogLevelName();
//@}

/**@name Global logging file control. */
//@{
void gSetLogFile(FILE*);
bool gSetLogFile(const char*);
//@}


#endif

// vim: ts=4 sw=4
