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
#include "GSMCommon.h"

int main() {

  USRPDevice *usrp = new USRPDevice(400.0e3); //533.333333333e3); //400e3);
  usrp->make();
  RadioInterface* radio = new RadioInterface(usrp,3);
  Transceiver trx(5700,"127.0.0.1",SAMPSPERSYM,GSM::Time(2,0),radio);
  trx.transmitFIFO(radio->transmitFIFO());
  trx.receiveFIFO(radio->receiveFIFO());

  trx.start();

  while(1) { sleep(1); }
}
