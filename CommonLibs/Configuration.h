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


#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <assert.h>
#include "Threads.h"
#include <map>
#include <vector>
#include <string>
#include <stdlib.h>
#include <iostream>


/** A class for configuration file errors. */
class ConfigurationTableError {};

/** An exception thrown when a given config key isn't found. */
class ConfigurationTableKeyNotFound : public ConfigurationTableError {

	private:

	std::string mKey;

	public:

	ConfigurationTableKeyNotFound(const std::string& wKey)
		:mKey(wKey)
	{ CERR("WARNING -- cannot find configuration value " << mKey); }
};



typedef std::map<std::string,std::string> StringMap;


/**
	A class for reading a configuration key-value table
	and storing it in a map.
	This class is not thread safe.
	The expectation is that the configuration table will be defined
	once and then used as read-only the rest of the time.
*/
class ConfigurationTable {

	private:

	StringMap mTable;

	public:

	bool readFile(const char* filename);

	ConfigurationTable(const char* filename)
		{ assert(readFile(filename)); }

	/**
		Return true if the key is used in the table.
	*/
	bool defines(const std::string& key) const;

	/**
		Get a string parameter from the table.
		Throw ConfigurationTableKeyNotFound if not found.
	*/
	const char *getStr(const std::string& key) const;

	/**
		Get a numeric parameter from the table.
		Throw ConfigurationTableKeyNotFound if not found.
	*/
	long getNum(const std::string& key) const { return strtol(getStr(key),NULL,10); }

	/**
		Get a numeric vector from the table.
	*/
	std::vector<unsigned> getVector(const std::string& key) const;

	/** Set or change a value in the table.  */
	void set(const std::string& key, const std::string& value)
		{ mTable[key]=value; }

	void set(const std::string& key, long value);

	/** Dump the table to a stream. */
	void dump(std::ostream&) const;

};

#endif


// vim: ts=4 sw=4
