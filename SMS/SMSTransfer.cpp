/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
*

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

* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*/

/*
Contributors:
Raffi Sevlian, raffisev@gmail.com
*/


#include "CommSig.h"
#include "SMSTransfer.h"

using namespace std;
using namespace SMS;




ostream& SMS::operator<<(ostream& os, SMSPrimitive prim)
{
	switch(prim) {
	
		case SM_RL_DATA_REQ: os<<" SM-RL-DATA-REQ"; break;
		case SM_RL_DATA_IND: os<<" SM-RL-DATA-IND"; break;
		case SM_RL_MEMORY_AVAIL_IND: os<<" SM-RL-MEMORY-AVAIL-IND"; break;
		case SM_RL_REPORT_REQ: os<<" SM-RL-REPORT-REQ"; break;
		case SM_RL_REPORT_IND: os<<" SM-RL-REPORT-IND"; break;
		case MNSMS_ABORT_REQ: os<<" MNSMS-ABORT-REQ"; break;
		case MNSMS_DATA_IND: os<<" MNSMS-DATA-IND"; break;
		case MNSMS_DATA_REQ: os<<"MNSMS-DATA-REQ "; break;
		case MNSMS_EST_REQ: os<<"MNSMS-EST-REQ "; break;
		case MNSMS_EST_IND: os<<"MNSMS-EST-IND "; break;
		case MNSMS_ERROR_IND: os<<"MNSMS-ERROR-IND "; break;
		case MNSMS_REL_REQ: os<<"MNSMS-REL-REQ "; break;
		case UNDEFINED_PRIMITIVE: os<<"undefined "; break;

	}
	return os;
}

ostream& SMS::operator<<(ostream& os, const RLFrame& msg)
{
	os<<" primitive="<<msg.primitive();
	os<<" data=("<<(const CommSig&)msg<<")";
	return os;
}

ostream& SMS::operator<<(ostream& os, const TLFrame& msg)
{
	os<<" primitive="<<msg.primitive();
	os<<" data=("<<(const CommSig&)msg<<")";
	return os;
}



