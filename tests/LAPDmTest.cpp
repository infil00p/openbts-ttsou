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
Contributors:
David A. Burgess, dburgess@ketsrelsp.com
*/


#include "GSML2LAPDm.h"
#include "GSMSAPMux.h"
#include "GSMTransfer.h"


/*
	Exercise LAPDm by creating two LAPDm entities and letting them
	exchange frames.
*/



using namespace GSM;

SDCCHL2 DL1(0);		// MS side
SDCCHL2 DL2(1);		// BTS side




int main(int argc, char *argv[])
{

	LoopbackSAPMux SAP1, SAP2;

	// DL1 -> SAP1 -> DL2
	SAP1.upstream(&DL2);
	DL1.downstream(&SAP1);

	// DL2 -> SAP2 -> DL1
	SAP2.upstream(&DL1);
	DL2.downstream(&SAP2);

	DL1.open();
	DL2.open();


	DL1.writeHighSide(ESTABLISH);
	for (int i=0; i<5; i++) {
		BitVector payload(40*8);
		payload.zero();
		payload.fillField(0,i,6);
		payload.fillField(20*8,i,6);
		L3Frame frame(payload,DATA);
		DL1.writeHighSide(frame);
	}
	DL1.writeHighSide(RELEASE);

	sleep(1);

	// pull frames out of DL2
	L3Frame *l3;
	while ((l3=DL2.readHighSide(0))) {
		COUT("DL2 L3 read " << *l3);
	}
	while ((l3=DL1.readHighSide(0))) {
		COUT("DL1 L3 read " << *l3);
	}
	
	sleep(2);
}
