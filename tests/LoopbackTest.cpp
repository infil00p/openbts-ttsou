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


#include "TRXManager.h"
#include "GSML1FEC.h"
#include "GSMConfig.h"
#include "GSMSAPMux.h"


using namespace GSM;


GSMConfig gBTS(0,0,EGSM900,L3LocationAreaIdentity("666","123",666),L3CellIdentity(0x1234),"WASTE");
TransceiverManager TRX(1, "127.0.0.1", 5700);


/// Start a test channel on the givn timeslot.
void *driveTask(int *TN)
{
	// The L1 test component.
	LoopbackL1FEC proc(*TN);

	// Downstream ARFCN manager
	// This is C0.
	ARFCNManager *ARFCN = TRX.ARFCN(0);
	// Treat as combination VII.
	ARFCN->setSlot(*TN,7);
	proc.downstream(ARFCN);

	// Upstream SAP mux
	L1TestPointSAPMux mux;
	proc.upstream(&mux);

	// Start the L1 and ready it for use.
	proc.open();

	// Build a L2 frame to testing.
	BitVector payload(16*8);
	for (unsigned i=0; i<payload.size(); i++) payload[i]=random()%2;
	L2Frame testFrame(payload, DATA);
	uint32_t count = 0;
	while (1) {
		// Put a counter near the front of the frame.
		testFrame.fillField(0,count<<16,64);
		count++;
		COUT("sending test frame " << testFrame);
		proc.writeHighSide(testFrame);
	}
	// DONTREACH
	pthread_exit(NULL);
}




int main(int argc, char *argv[])
{
	TRX.start();

	// Set up the interface to the radio.
	ARFCNManager* ARFCN = TRX.ARFCN(0);
      	ARFCN->tuneLoopback(29);
       	ARFCN->setTSC(0);
       	ARFCN->setPower(10);

	// Start the threads that run the loopback test processors.
	int index[8] = {1,0,0,0,0,0,0,0};
	Thread driver[8];
	for (int i=0; i<8; i++) {
		if (index[i]) {
			index[i]=i;
			driver[i].start((void*(*)(void*))driveTask,(void*)&index[i]);
		}
	}

	// Just sleep now.
	while (1) { sleep(1); }
}
