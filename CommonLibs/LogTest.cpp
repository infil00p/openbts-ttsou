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

#include "Logger.h"


int main(int argc, char *argv[])
{
	gSetLogLevel("INFO");
	LOG(ERROR) << " testing the logger.";
	LOG(WARN) << " testing the logger.";
	LOG(NOTICE) << " testing the logger.";
	LOG(INFO) << " testing the logger.";
	LOG(DEBUG) << " testing the logger.";
	LOG(DEEPDEBUG) << " testing the logger.";
}



