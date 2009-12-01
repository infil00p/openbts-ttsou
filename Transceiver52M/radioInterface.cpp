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

//#define NDEBUG
#include "radioInterface.h"
#include <Logger.h>


GSM::Time VectorQueue::nextTime() const
{
  GSM::Time retVal;
  mLock.lock();
  while (mQ.size()==0) mWriteSignal.wait(mLock);
  retVal = mQ.top()->time();
  mLock.unlock();
  return retVal;
}

radioVector* VectorQueue::getStaleBurst(const GSM::Time& targTime)
{
  mLock.lock();
  if ((mQ.size()==0)) {
    mLock.unlock();
    return NULL;
  }
  if (mQ.top()->time() < targTime) {
    radioVector* retVal = mQ.top();
    mQ.pop();
    mLock.unlock();
    return retVal;
  }
  mLock.unlock();
  return NULL;
}


radioVector* VectorQueue::getCurrentBurst(const GSM::Time& targTime)
{
  mLock.lock();
  if ((mQ.size()==0)) {
    mLock.unlock();
    return NULL;
  }
  if (mQ.top()->time() == targTime) {
    radioVector* retVal = mQ.top();
    mQ.pop();
    mLock.unlock();
    return retVal;
  }
  mLock.unlock();
  return NULL;
}



RadioInterface::RadioInterface(USRPDevice *wUsrp,
                               int wReceiveOffset,
			       int wSamplesPerSymbol,
			       GSM::Time wStartTime)

{
  underrun = false;
 
  sendCursor = 0; 
  rcvCursor = 0;
  mOn = false;
  
  usrp = wUsrp;
  receiveOffset = wReceiveOffset;
  samplesPerSymbol = wSamplesPerSymbol;
  mClock.set(wStartTime);
  powerScaling = 1.0;
}

RadioInterface::~RadioInterface(void) {
  if (rcvBuffer!=NULL) delete rcvBuffer;
  //mReceiveFIFO.clear();
}

short *RadioInterface::USRPifyVector(signalVector &wVector, short *retVector) 
{


  signalVector::iterator itr = wVector.begin();
  short *shortItr = retVector;
  while (itr < wVector.end()) {
    *shortItr++ = (short) host_to_usrp_short(itr->real());
    *shortItr++ = (short) host_to_usrp_short(itr->imag());
    itr++;
  }

  return retVector;

}

void RadioInterface::unUSRPifyVector(short *shortVector, signalVector& newVector)
{
  
  signalVector::iterator itr = newVector.begin();
  short *shortItr = shortVector;

// need to flip I and Q from USRP
#ifndef SWLOOPBACK
#define FLIP_IQ 1
#else
#define FLIP_IQ 0
#endif

  while (itr < newVector.end()) {
    *itr++ = Complex<float>(usrp_to_host_short(*(shortItr+FLIP_IQ)),
		            usrp_to_host_short(*(shortItr+1-FLIP_IQ)));
    //LOG(DEEPDEBUG) << (*(itr-1));
    shortItr += 2;
  }

}


bool started = false;

void RadioInterface::pushBuffer(void) {

  if (sendCursor < 2*INCHUNK) return;

  // start the USRP when we actually have data to send to the USRP.
  if (!started) {
    started = true; 
    LOG(INFO) << "Starting USRP";
    usrp->start(); 
    LOG(DEBUG) << "USRP started";
    usrp->updateAlignment(10000); 
    usrp->updateAlignment(10000);
  }

  // send resampleVector
  int samplesWritten = usrp->writeSamples(sendBuffer,
					  INCHUNK,
					  &underrun,
					  writeTimestamp); 
  //LOG(DEEPDEBUG) << "writeTimestamp: " << writeTimestamp << ", samplesWritten: " << samplesWritten;
   
  writeTimestamp += (TIMESTAMP) samplesWritten;

  if (sendCursor > 2*samplesWritten) 
    memcpy(sendBuffer,sendBuffer+samplesWritten*2,sizeof(short)*2*(sendCursor-2*samplesWritten));
  sendCursor = sendCursor - 2*samplesWritten;
}


void RadioInterface::pullBuffer(void)
{
   
  bool localUnderrun;

   // receive receiveVector
  short* shortVector = rcvBuffer+rcvCursor;  
  int samplesRead = usrp->readSamples(shortVector,OUTCHUNK,&overrun,readTimestamp,&localUnderrun);
  underrun |= localUnderrun;
  readTimestamp += (TIMESTAMP) samplesRead;
  while (samplesRead < OUTCHUNK) {
    int oldSamplesRead = samplesRead;
    samplesRead += usrp->readSamples(shortVector+2*samplesRead,
				     OUTCHUNK-samplesRead,
				     &overrun,
				     readTimestamp,
				     &localUnderrun);
    underrun |= localUnderrun;
    readTimestamp += (TIMESTAMP) (samplesRead - oldSamplesRead);
  }
  LOG(DEBUG) << "samplesRead " << samplesRead;

  rcvCursor += samplesRead*2;

}

bool RadioInterface::tuneTx(double freq)
{
  if (mOn) return false;
  return usrp->setTxFreq(freq);
}

bool RadioInterface::tuneRx(double freq)
{
  if (mOn) return false;
  return usrp->setRxFreq(freq);
}


void RadioInterface::start()
{
  LOG(INFO) << "starting radio interface...";
  mAlignRadioServiceLoopThread.start((void * (*)(void*))AlignRadioServiceLoopAdapter,
                                     (void*)this);
  mOn = true;
  writeTimestamp = 20000;
  readTimestamp = 20000;
  LOG(DEBUG) << "radio interface started!";
}

void *AlignRadioServiceLoopAdapter(RadioInterface *radioInterface)
{
  while (1) {
    radioInterface->alignRadio();
    pthread_testcancel();
  }
  return NULL;
}

void RadioInterface::alignRadio() {
  sleep(60);
  usrp->updateAlignment(writeTimestamp+ (TIMESTAMP) 10000);
}

void RadioInterface::driveTransmitRadio(signalVector &radioBurst) {

  if (!mOn) return;

  if (powerScaling != 1.0) scaleVector(radioBurst, powerScaling);

  USRPifyVector(radioBurst, sendBuffer+sendCursor);

  sendCursor += (radioBurst.size()*2);

  pushBuffer();
}

void RadioInterface::driveReceiveRadio() {

  if (!mOn) return;

  if (mReceiveFIFO.size() > 8) return;

  pullBuffer();

  GSM::Time rcvClock = mClock.get();
  rcvClock.decTN(receiveOffset);
  unsigned tN = rcvClock.TN();
  int rcvSz = rcvCursor/2;
  int readSz = 0;
  const int symbolsPerSlot = gSlotLen + 8;

  // while there's enough data in receive buffer, form received 
  //    GSM bursts and pass up to Transceiver
  // Using the 157-156-156-156 symbols per timeslot format.
  while (rcvSz > (symbolsPerSlot + (tN % 4 == 0))*samplesPerSymbol) {
    signalVector rxVector((symbolsPerSlot + (tN % 4 == 0)*samplesPerSymbol));
    unUSRPifyVector(rcvBuffer+readSz*2,rxVector);
    GSM::Time tmpTime = rcvClock;
    if (rcvClock.FN() >= 0) {
      LOG(DEEPDEBUG) << "FN: " << rcvClock.FN();
      radioVector* rxBurst = new radioVector(rxVector,tmpTime);
      mReceiveFIFO.put(rxBurst); 
    }
    mClock.incTN(); 
    rcvClock.incTN();
    //if (mReceiveFIFO.size() >= 16) mReceiveFIFO.wait(8);
    LOG(DEBUG) << "receiveFIFO: wrote radio vector at time: " << mClock.get() << ", new size: " << mReceiveFIFO.size() ;
    readSz += (symbolsPerSlot+(tN % 4 == 0))*samplesPerSymbol;
    rcvSz -= (symbolsPerSlot+(tN % 4 == 0))*samplesPerSymbol;

    tN = rcvClock.TN();
  }

  if (readSz > 0) { 
    memcpy(rcvBuffer,rcvBuffer+2*readSz,sizeof(short)*2*(rcvCursor-readSz));
    rcvCursor = rcvCursor-2*readSz;
  }
} 
  
