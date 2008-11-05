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



#include "Threads.h"
#include "Interthread.h"
#include "Sockets.h"
#include "GSMCommon.h"


using namespace GSM;


Clock gClock;

unsigned basePort = 5700;
UDPSocket gClockSocket(basePort,"127.0.0.1",basePort+100);
UDPSocket controlSocket(basePort+1,"127.0.0.1",basePort+101);
UDPSocket dataSocket(basePort+2,"127.0.0.1",basePort+102);

Mutex gClockSocketLock;
volatile unsigned gLeadFrames=0;
volatile float fLeadFrames = 0.0F;




void sendTime()
{
	/* send a clock packet with lead-offset */
	char buf[100];
	uint32_t FN = gClock.get().FN();
	sprintf(buf,"IND CLOCK %u",(FN+gLeadFrames)%gHyperframe);
	DCOUT("CLOCK packet at " << FN << " lead " << gLeadFrames);
	gClockSocketLock.lock();
	gClockSocket.write(buf);
	gClockSocketLock.unlock();
}



void *controlTask()
{
	/* echo back a response for every command */
	/* also send a clock packet whenever we respond to a command */
	while (1) {
		char buffer[MAX_UDP_LENGTH];
		controlSocket.read(buffer);
		DCOUT("cntrl read " << buffer);
		char cmd1[20], cmd2[20];
		sscanf(buffer, "%20s %20s", cmd1, cmd2);
		int cmdLen = 1+sprintf(buffer, "RSP %s 0", cmd2);
		DCOUT("cntrl write " << buffer);
		controlSocket.write(buffer,cmdLen);
		if (strcmp(cmd2,"CLOCK")) {
			gLeadFrames=0;
			fLeadFrames=0.0;
			sendTime();
		}
	}
	return NULL;
}


void *dataTask()
{
	unsigned totalFrames=0;
	unsigned lateFrames=0;


	while (1) {
		/* read the transmit burst */
		char txBuf[MAX_UDP_LENGTH];
		dataSocket.read(txBuf);
		/* copy over everything but the data */
		char rxBuf[MAX_UDP_LENGTH];
		bcopy(txBuf,rxBuf,6);
		rxBuf[6]=0; rxBuf[7]=0;
		/* check the clock */
		totalFrames++;
		uint32_t txFN = 0;
		for (int i=0; i<4; i++) txFN = (txFN<<8) + (unsigned char)txBuf[i+1];
		uint32_t myFN = gClock.get().FN();
		/* tweak lead offset as needed */
		const float lateTarg = 0.005;		// allow up to 0.5% late frames
		const float latePenalty = 4.0;		// late frame 4x as bad as early one
		const unsigned window = 51*26;		// analysis window length in frames
		float lateFrac = ((float)lateFrames)/((float)totalFrames);
		float leadStep = lateFrac;
		if (FNCompare(txFN,myFN)<0) {
			DCOUT("LATE FRAME for " << txFN << " at " << myFN);
			lateFrames++;
			fLeadFrames += latePenalty*leadStep;
			// echo back unknown bits for late frame
			for (int i=0; i<148; i++) {
				rxBuf[i+8]=128;
			}
		} else {
			if (lateFrac<lateTarg) fLeadFrames -= lateTarg;
			/* copy data and echo back as a received burst */
			for (int i=0; i<148; i++) {
				if (txBuf[i+6]) rxBuf[i+8]=255;
				else rxBuf[i+8]=0;
				/* replace 10% of bits with garbage */
				//if (random()%10==0) rxBuf[i+8]=(random()%256);
			}
		}
		/* send the simulated received data */
		dataSocket.write(rxBuf,148+4+2+2);
		/* if the integer version of the lead offset has changed, send a clock packet */
		if (fLeadFrames<0.0F) fLeadFrames=0;
		unsigned iLeadFrames = (int)fLeadFrames;
		if (iLeadFrames!=gLeadFrames) {
			gLeadFrames=iLeadFrames;
			DCOUT("CLOCK packet for " << txFN << " at " << myFN <<
				" lead " << gLeadFrames << " late " << lateFrac);
			sendTime();
		}
		/* update frame counts over the window */
		if (totalFrames>window) {
			totalFrames /= 2;
			lateFrames /= 2;
		}
	}
	return NULL;
}





int main(int argc, char *argv[])
{

	gClock.set(GSM::gHyperframe-2000);
	Thread controlThread;
	controlThread.start((void*(*)(void*))controlTask,NULL);
	Thread dataThread;
	dataThread.start((void*(*)(void*))dataTask,NULL);

	while (1) {
		sleep(1);
		DCOUT("current clock is " << gClock.get());
	}
}
