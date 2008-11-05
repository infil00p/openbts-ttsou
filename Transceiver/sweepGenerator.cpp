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

using namespace std;

int main() {

  USRPDevice *usrp=new USRPDevice(10*400e3);
  if (!usrp->make(true)) {
    exit(1);
  }

  float baseFreq = 930.0e6;
  usrp->setTxFreq(baseFreq);    

  int writeTimestamp = 10000;
  bool underrun;

  int offset = RAND_MAX/2;
  float scale = 65000.0/(float) offset;
  short* pulse = (short*) calloc(1,sizeof(short)*400);
  for (int i = 0; i < 400; i++) {
    pulse[i] = scale*(float)(rand()-offset);
  }

  usrp->start();
  while(1) {
    writeTimestamp += usrp->writeSamples(pulse,200,&underrun,0x0ffffffff);
    usrp->setTxFreq(baseFreq);    
    baseFreq += 4.0e6;
    if (baseFreq > 960.0e6) baseFreq = 930.0e6;
  }
}
    
