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


#include "Configuration.h"
#include <Logger.h>
#include <fstream>
#include <iostream>
#include <string.h>

using namespace std;

bool ConfigurationTable::readFile(const char* filename)
{
	ifstream configFile(filename);
	if (!configFile) {
		LOG(WARN) << "cannot open configuration file " << filename;
		return false;
	}
	while (configFile) {
		string thisLine;
		getline(configFile,thisLine);
		if (!configFile) break;
		// Skip leading spaces.
		int i=0;
		while (thisLine[i]==' ') i++;
		// Skip comments
		if (thisLine[i]=='#') continue;
		// Skip blank lines
		if (thisLine[i]=='\0') continue;
		// Tokenize and put in the table.
		string::size_type pos = thisLine.find_first_of(" ",i);
		if (pos==string::npos) {
			mTable[thisLine]="";
			continue;
		}
		string key = thisLine.substr(0,pos);
		string value = thisLine.substr(pos+1);
		mTable[key]=value;
		LOG(DEBUG) << "configuring " << key << " = " << value;
	}
	configFile.close();
	return true;
}



bool ConfigurationTable::defines(const string& key) const
{
	StringMap::const_iterator where = mTable.find(key);
	return (where!=mTable.end());
}



const char* ConfigurationTable::getStr(const string& key) const
{
	StringMap::const_iterator where = mTable.find(key);
	if (where==mTable.end()) throw ConfigurationTableKeyNotFound(key);
	return where->second.c_str();
}


std::vector<unsigned> ConfigurationTable::getVector(const string& key) const
{
	StringMap::const_iterator where = mTable.find(key);
	if (where==mTable.end()) throw ConfigurationTableKeyNotFound(key);
	// Make an alterable copy of the string.
	char* line = strdup(where->second.c_str());
	std::vector<unsigned> retVal;
	char *lp=line;
	while (lp) {
		retVal.push_back(strtol(lp,NULL,10));
		strsep(&lp," ");
	}
	free(line);
	return retVal;
}


void ConfigurationTable::dump(ostream& os) const
{
	StringMap::const_iterator cfg = mTable.begin();
	while (cfg != mTable.end()) {
		os << cfg->first << ": " << cfg->second << endl;
		++cfg;
	}
}

void ConfigurationTable::set(const string& key, long value)
{
	char buffer[30];
	sprintf(buffer,"%ld",value);
	set(key,buffer);
}


// vim: ts=4 sw=4
