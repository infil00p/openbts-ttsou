/*
* Copyright 2008 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.

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



/*
	This test sets of a pager and sends paging messages
	on a CCCH.
	It does not require a radio.
*/

#include "ControlCommon.h"
#include "GSML1FEC.h"
#include "TRXManager.h"
#include "GSMLogicalChannel.h"
#include "GSMConfig.h"
#include "SIPInterface.h"


using namespace std;
using namespace GSM;
using namespace SIP;
using namespace Control;



GSMConfig gBTS(0,0,GSM850,L3LocationAreaIdentity("666","333",666),1234,"test");
SIPInterface gSIPInterface(SIP_UDP_PORT, "0.0.0.0", 5060);

int main(int argc, char *argv[])
{
	CCCHLogicalChannel CCCH(gCCCH_0Mapping);
	TransceiverManager TRX(1,"localhost",5700);

	gBTS.addPCH(&CCCH);
	CCCH.downstream(TRX.ARFCN(0));
	Pager pager;

	TRX.start();
	CCCH.open();
	pager.start();

	while (1) {
		pager.addID(L3MobileIdentity(random()),GSM::AnyDCCHType);
		pager.addID(L3MobileIdentity(random()),GSM::AnyDCCHType);
		pager.addID(L3MobileIdentity("123456789012345"),GSM::AnyDCCHType);
		sleep(random() % 2);
	}
}
