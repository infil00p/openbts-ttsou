/**@file Global system parameters. */
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

#include "config.h"
#include <Globals.h>
#include <CLI.h>


const char* gOpenBTSWelcome =
	//23456789123456789223456789323456789423456789523456789623456789723456789
	"OpenBTS, Copyright 2008, 2009 Free Software Foundation, Inc.\n"
	"Version: " VERSION "\n"
	"Contributors:\n"
	"\tKestrel Signal Processing, Inc.:\n"
	"\t\tDavid Burgess, Harvind Samra, Raffi Sevlian, Roshan Baliga\n"
	"\tGNU Radio:\n"
	"\t\tJohnathan Corgan\n"
	"\tOthers:\n"
	"\t\tAnne Kwong, Jacob Appelbaum, Alberto Escudero-Pascual\n"
	"\tIncorporated GPL libraries and components:\n"
	"\t\tlibosip2, liportp2\n"
	"This program comes with ABSOLUTELY NO WARRANTY.\n"
	"This is free software;\n"
	"you are welcome to redistribute it under the terms of GPLv3.\n"
	"Use of this software may be subject to other legal restrictions,\n"
	"including patent licsensing and radio spectrum licensing.\n"
	"All users of this software are expected to comply with\n"
	"applicable regulations.\n"
;


CommandLine::Parser gParser;
