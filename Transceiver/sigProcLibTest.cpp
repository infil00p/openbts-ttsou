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



#include "sigProcLib.h"
//#include "radioInterface.h"

using namespace std;

int main(int argc, char **argv) {

  int samplesPerSymbol = 1;

  sigProcLibSetup(samplesPerSymbol);
  
  signalVector *gsmPulse = generateGSMPulse(2,samplesPerSymbol);
  cout << *gsmPulse << endl;

  CommSig RACHBurstStart = "01010101";
  CommSig RACHBurstRest = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

  CommSig RACHBurst(CommSig(RACHBurstStart,gRACHSynchSequence),RACHBurstRest);
 

  signalVector *RACHSeq = modulateBurst(RACHBurst,
                                        *gsmPulse,
                                        9,
                                        samplesPerSymbol);

  generateRACHSequence(*gsmPulse,samplesPerSymbol);

  complex a; float t;
  detectRACHBurst(*RACHSeq, 5, samplesPerSymbol,&a,&t); 

  //cout << *RACHSeq << endl;
  //signalVector *autocorr = correlate(RACHSeq,RACHSeq,NULL,NO_DELAY);

  //cout << *autocorr;

  //exit(1);
 

  /*signalVector x(6500);
  x.fill(1.0);

  frequencyShift(&x,&x,0.48*M_PI);

  signalVector *y = polyphaseResampleVector(x,96,65,NULL);

  cout << *y << endl;
 
  exit(1);*/

  //CommSig normalBurstSeg = "0000000000000000000000000000000000000000000000000000000000000";

  CommSig normalBurstSeg = "0000101010100111110010101010010110101110011000111001101010000";

  CommSig normalBurst(CommSig(normalBurstSeg,gTrainingSequence[0]),normalBurstSeg);

  generateMidamble(*gsmPulse,samplesPerSymbol,0);

  signalVector *modBurst = modulateBurst(normalBurst,*gsmPulse,
					 0,samplesPerSymbol);  
  int OUTRATE = 96;
  int INRATE = 65*samplesPerSymbol;
  // delay is POLYPHASESPAN/2/INRATE
  int P = OUTRATE; int Q = INRATE;
  int POLYPHASESPAN = 651;
  float cutoffFreq = (P < Q) ? (1.0/(float) Q) : (1.0/(float) P);
  signalVector *sendLPF = createLPF(cutoffFreq,POLYPHASESPAN,P);

  COUT("sendLPFsz: " << sendLPF->size());

  P = INRATE; Q = OUTRATE;
  POLYPHASESPAN = 961;
  cutoffFreq = (P < Q) ? (1.0/(float) Q) : (1.0/(float) P);
  signalVector *rcvLPF = createLPF(cutoffFreq,POLYPHASESPAN,P);

  signalVector modBurst2(*modBurst,*modBurst);
  //modBurst->fill(1.0);
  //frequencyShift(modBurst,modBurst,2.0*M_PI/(8.0*(float) INRATE/(float)OUTRATE));
 

  signalVector *rsVector = polyphaseResampleVector(*modBurst,
						   OUTRATE,
						   INRATE,
						   sendLPF);

  scaleVector(*rsVector,1.0); //4096.0*(50*POLYPHASESPAN+1));

  signalVector *rsVector2 = polyphaseResampleVector(*rsVector,
						    INRATE,OUTRATE,rcvLPF);
  
  cout << "rsVector2 size" << rsVector2->size() << endl;
  signalVector *autocorr = correlate(rsVector2,RACHSeq,NULL,NO_DELAY);

  //cout << "cutoffFreq: " << cutoffFreq << endl;
  cout << *modBurst << endl;
  cout << "Energy: " << vectorNorm2(*rsVector) << endl;
  cout << *rsVector << endl;
  cout << *rsVector2 << endl;
  
  delayVector(*rsVector2,6.932);

  complex ampl = 1;
  float TOA = 0;

  modBurst = rsVector2;
  //delayVector(*modBurst,0*7.5);

  signalVector channelResponse(4);
  signalVector::iterator c=channelResponse.begin();
  *c = (complex) 9000.0; c++;
  *c = (complex) 0.4*9000.0; c++; c++;
  *c = (complex) -1.2*0;

  signalVector *guhBurst = convolve(modBurst,&channelResponse,NULL,NO_DELAY);
  delete modBurst; modBurst = guhBurst;
  
  signalVector *chanResp;
  double noisePwr = 0.001/sqrtf(2);
  signalVector *noise = gaussianNoise(modBurst->size(),noisePwr);
  float chanRespOffset;
  analyzeTrafficBurst(*modBurst,0,8.0,samplesPerSymbol,&ampl,&TOA,true,&chanResp,&chanRespOffset);
  addVector(*modBurst,*noise);

  cout << "ampl:" << ampl << endl;
  cout << "TOA: " << TOA << endl;
  //cout << "chanResp: " << *chanResp << endl;
  SoftSig *demodBurst = demodulateBurst(*modBurst,*gsmPulse,samplesPerSymbol,(complex) ampl, TOA);
  
  cout << *demodBurst << endl;

  COUT("chanResp: " << *chanResp);

  signalVector *w,*b;
  designDFE(*chanResp,1.0/noisePwr,7,&w,&b); 
  COUT("w: " << *w);
  COUT("b: " << *b);

 
  SoftSig *DFEBurst = equalizeBurst(*modBurst,TOA-chanRespOffset,samplesPerSymbol,*w,*b);
  COUT("DFEBurst: " << *DFEBurst);

  delete gsmPulse;
  delete RACHSeq;
  delete modBurst;
  delete sendLPF;
  delete rcvLPF;
  delete rsVector;
  //delete rsVector2;
  delete autocorr;
  delete chanResp;
  delete noise;
  delete demodBurst;
  delete w;
  delete b;
  delete DFEBurst;  

  sigProcLibDestroy();

}
