/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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
Harvind S. Samra, hssamra@kestrelsp.com
*/


#include "Transceiver.h"
#include "TRXManager.h"
#include "GSMConfig.h"
#include "GSMCommon.h"
#include "ControlCommon.h"

using namespace std;
using namespace Control;
using namespace GSM;

TransactionTable gTransactionTable;

GSMConfig gBTS(0,0,EGSM900);

int main() {

  TransceiverManager *TRX = new TransceiverManager(1,gBTS,"127.0.0.1",5600);

  USRPDevice *usrp = new USRPDevice(400e3);
  if (!usrp->make()) {
	CERR("can't open USRP!");
	exit(1);
  }

  RadioInterface* radio = new RadioInterface(usrp,0);
  Transceiver *trx = new Transceiver(5600,"127.0.0.1",SAMPSPERSYM,GSMTime(2,0),radio);
  trx->transmitFIFO(radio->transmitFIFO());
  trx->receiveFIFO(radio->receiveFIFO());
 
  ARFCNManager *arfcn = TRX->ARFCN(0);

  TRX->start();

  trx->start();

  arfcn->tune(0);

  arfcn->setTSC(6);

  arfcn->setPower(20);

  int numActiveSlots = 8;

  for (int i = 0; i < numActiveSlots; i++) {
    arfcn->setSlot(i,1);
    usleep(20000);
  }

  GSMTime theClock(0);

  bool zeros = true;

  while (1) {
    TRX->config().clock().wait(theClock);
    CommSig normalBurstSeg(61);
    CommSig normalBurstSegInv(61);
    for(int i = 0; i < 61; i++) {
      normalBurstSeg[i] = 0; //random() % 2;
      normalBurstSegInv[i] = 1; 
    }

    CommSig midamble(gTrainingSequence[6]);
    CommSig midambleInv(gTrainingSequence[6]);

    CommSig normalBurst(CommSig(normalBurstSeg,midamble),normalBurstSeg);
    CommSig normalBurstInv(CommSig(normalBurstSegInv,midambleInv),normalBurstSegInv);

    for (int i = 0; i < 8;i++) {
	TxBurst newBurst(normalBurst,theClock);
	if (i<numActiveSlots) arfcn->writeHighSide(newBurst);
        theClock.incTN();
    }

  }

  cout << "Sleeping...\n";

  sleep(10);

  cout << "Shutting down!!!!\n";

  //trx->stop();

  //TRX->stop();	

  delete arfcn;

  delete TRX;

  delete usrp;

  delete trx;

  delete TRX;
 
  delete radio;

  exit(1);

}

  
