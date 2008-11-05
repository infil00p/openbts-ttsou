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



#define NDEBUG

#include "sigProcLib.h"
#include "GSMCommon.h"
#include "sendLPF_961.h"
#include "rcvLPF_651.h"

#define TABLESIZE 1024

/** Lookup tables for trigonometric approximation */
float cosTable[TABLESIZE+1]; // add 1 element for wrap around
float sinTable[TABLESIZE+1];

/** Constants */
static const float M_PI_F = (float)M_PI;
static const float M_2PI_F = (float)(2.0*M_PI);
static const float M_1_2PI_F = 1/M_2PI_F;

/** Static vectors that contain a precomputed +/- f_b/4 sinusoid */ 
signalVector *GMSKRotation = NULL;
signalVector *GMSKReverseRotation = NULL;

/** Static ideal RACH and midamble correlation waveforms */
typedef struct {
  signalVector *sequence;
  float        TOA;
  complex      gain;
} CorrelationSequence;

CorrelationSequence *gMidambles[] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
CorrelationSequence *gRACHSequence = NULL;

void sigProcLibDestroy(void) {
  if (GMSKRotation) {
    delete GMSKRotation;
    GMSKRotation = NULL;
  }
  if (GMSKReverseRotation) {
    delete GMSKReverseRotation;
    GMSKReverseRotation = NULL;
  }
  for (int i = 0; i < 8; i++) {
    if (gMidambles[i]!=NULL) {
      if (gMidambles[i]->sequence) delete gMidambles[i]->sequence;
      delete gMidambles[i];
      gMidambles[i] = NULL;
    }
  }
  if (gRACHSequence) {
    if (gRACHSequence->sequence) delete gRACHSequence->sequence;
    delete gRACHSequence;
    gRACHSequence = NULL;
  }
}



// dB relative to 1.0.
// if > 1.0, then return 0 dB
float dB(float x) {
  
  float arg = 1.0F;
  float dB = 0.0F;
  
  if (x >= 1.0F) return 0.0F;
  if (x <= 0.0F) return -200.0F;

  float prevArg = arg;
  float prevdB = dB;
  float stepSize = 16.0F;
  float dBstepSize = 12.0F;
  while (stepSize > 1.0F) {
    do {
      prevArg = arg;
      prevdB = dB;
      arg /= stepSize;
      dB -= dBstepSize;
    } while (arg > x);
    arg = prevArg;
    dB = prevdB;
    stepSize *= 0.5F;
    dBstepSize -= 3.0F;
  }
 return ((arg-x)*(dB-3.0F) + (x-arg*0.5F)*dB)/(arg - arg*0.5F);

}

// 10^(-dB/10), inverse of dB func.
float dBinv(float x) {
  
  float arg = 1.0F;
  float dB = 0.0F;
  
  if (x >= 0.0F) return 1.0F;
  if (x <= -200.0F) return 0.0F;

  float prevArg = arg;
  float prevdB = dB;
  float stepSize = 16.0F;
  float dBstepSize = 12.0F;
  while (stepSize > 1.0F) {
    do {
      prevArg = arg;
      prevdB = dB;
      arg /= stepSize;
      dB -= dBstepSize;
    } while (dB > x);
    arg = prevArg;
    dB = prevdB;
    stepSize *= 0.5F;
    dBstepSize -= 3.0F;
  }

  return ((dB-x)*(arg*0.5F)+(x-(dB-3.0F))*(arg))/3.0F;

}

float vectorNorm2(const signalVector &x) 
{
  signalVector::const_iterator xPtr = x.begin();
  float Energy = 0.0;
  for (;xPtr != x.end();xPtr++) {
	Energy += xPtr->norm2();
  }
  return Energy;
}


float vectorPower(const signalVector &x) 
{
  return vectorNorm2(x)/x.size();
}

/** compute cosine via lookup table */
float cosLookup(const float x)
{
  float arg = x*M_1_2PI_F;
  while (arg > 1.0F) arg -= 1.0F;
  while (arg < 0.0F) arg += 1.0F;

  const float argT = arg*((float)TABLESIZE);
  const int argI = (int)argT;
  const float delta = argT-argI;
  const float iDelta = 1.0F-delta;
  return iDelta*cosTable[argI] + delta*cosTable[argI+1];
}

/** compute sine via lookup table */
float sinLookup(const float x) 
{
  float arg = x*M_1_2PI_F;
  while (arg > 1.0F) arg -= 1.0F;
  while (arg < 0.0F) arg += 1.0F;

  const float argT = arg*((float)TABLESIZE);
  const int argI = (int)argT;
  const float delta = argT-argI;
  const float iDelta = 1.0F-delta;
  return iDelta*sinTable[argI] + delta*sinTable[argI+1];
}


/** compute e^(-jx) via lookup table. */
complex expjLookup(float x)
{
  float arg = x*M_1_2PI_F;
  while (arg > 1.0F) arg -= 1.0F;
  while (arg < 0.0F) arg += 1.0F;

  const float argT = arg*((float)TABLESIZE);
  const int argI = (int)argT;
  const float delta = argT-argI;
  const float iDelta = 1.0F-delta;
   return complex(iDelta*cosTable[argI] + delta*cosTable[argI+1],
		   iDelta*sinTable[argI] + delta*sinTable[argI+1]);
}

/** Library setup functions */
void initTrigTables() {
  for (int i = 0; i < TABLESIZE+1; i++) {
    cosTable[i] = cos(2.0*M_PI*i/TABLESIZE);
    sinTable[i] = sin(2.0*M_PI*i/TABLESIZE);
  }
}

void initGMSKRotationTables(int samplesPerSymbol) {
  GMSKRotation = new signalVector(157*samplesPerSymbol);
  GMSKReverseRotation = new signalVector(157*samplesPerSymbol);
  signalVector::iterator rotPtr = GMSKRotation->begin();
  signalVector::iterator revPtr = GMSKReverseRotation->begin();
  float phase = 0.0;
  while (rotPtr != GMSKRotation->end()) {
    *rotPtr++ = expjLookup(phase);
    *revPtr++ = expjLookup(-phase);
    phase += M_PI_F/2.0F/(float) samplesPerSymbol;
  }
}

void sigProcLibSetup(int samplesPerSymbol) {
  initTrigTables();
  initGMSKRotationTables(samplesPerSymbol);
}

void GMSKRotate(signalVector &x) {
  signalVector::iterator xPtr = x.begin();
  signalVector::iterator rotPtr = GMSKRotation->begin();
  if (x.isRealOnly()) {
    while (xPtr < x.end()) {
      *xPtr = *rotPtr++ * (xPtr->real());
      xPtr++;
    }
  }
  else {
    while (xPtr < x.end()) {
      *xPtr = *rotPtr++ * (*xPtr);
      xPtr++;
    }
  }
}

void GMSKReverseRotate(signalVector &x) {
  signalVector::iterator xPtr= x.begin();
  signalVector::iterator rotPtr = GMSKReverseRotation->begin();
  if (x.isRealOnly()) {
    while (xPtr < x.end()) {
      *xPtr = *rotPtr++ * (xPtr->real());
      xPtr++;
    }
  }
  else {
    while (xPtr < x.end()) {
      *xPtr = *rotPtr++ * (*xPtr);
      xPtr++;
    }
  }
}


signalVector* convolve(const signalVector *a,
		       const signalVector *b,
		       signalVector *c,
		       ConvType spanType)
{
  if ((a==NULL) || (b==NULL)) return NULL; 
  int La = a->size();
  int Lb = b->size();

  int startIndex;
  unsigned int outSize;
  switch (spanType) {
    case FULL_SPAN:
      startIndex = 0;
      outSize = La+Lb-1;
      break;
    case OVERLAP_ONLY:
      startIndex = La;
      outSize = abs(La-Lb)+1;
      break;
    case START_ONLY:
      startIndex = 0;
      outSize = La;
      break;
    case WITH_TAIL:
      startIndex = Lb;
      outSize = La;
      break;
    case NO_DELAY:
      if (Lb % 2) 
	startIndex = Lb/2;
      else
	startIndex = Lb/2-1;
      outSize = La;
      break;
    default:
      return NULL;
  }

  
  if (c==NULL)
    c = new signalVector(outSize);
  else if (c->size()!=outSize)
    return NULL;

  signalVector::const_iterator aStart = a->begin();
  signalVector::const_iterator bStart = b->begin();
  signalVector::const_iterator aEnd = a->end();
  signalVector::const_iterator bEnd = b->end();
  signalVector::iterator cPtr = c->begin();
  int t = startIndex;
  int stopIndex = startIndex + outSize;
  switch (b->getSymmetry()) {
  case NONE:
    {
      while (t < stopIndex) {
	signalVector::const_iterator aP = aStart+t;
	signalVector::const_iterator bP = bStart;
	if (a->isRealOnly() && b->isRealOnly()) {
	  float sum = 0.0;
	  while (bP < bEnd) {
	    if (aP < aStart) break;
	    if (aP < aEnd) sum += (aP->real())*(bP->real());
	    aP--;
	    bP++;
	  }
	  *cPtr++ = sum;
	}
	else if (a->isRealOnly()) {
	  complex sum = 0.0;
	  while (bP < bEnd) {
	    if (aP < aStart) break;
	    if (aP < aEnd) sum += (*bP)*(aP->real());
	    aP--;
	    bP++;
	  }
	  *cPtr++ = sum;
	}
	else if (b->isRealOnly()) {
	  complex sum = 0.0;
	  while (bP < bEnd) {
	    if (aP < aStart) break;
	    if (aP < aEnd) sum += (*aP)*(bP->real());
	    aP--;
	    bP++;
	  }
	  *cPtr++ = sum;
	}
	else {
	  complex sum = 0.0;
	  while (bP < bEnd) {
	    if (aP < aStart) break;
	    if (aP < aEnd) sum += (*aP)*(*bP);
	    aP--;
	    bP++;
	  }
	  *cPtr++ = sum;
	}
	t++;
      }
    }
    break;
  case ABSSYM:
    {
      complex sum = 0.0;
      bool isOdd = (bool) (Lb % 2);
      if (isOdd) 
	bEnd = bStart + (Lb+1)/2;
      else 
	bEnd = bStart + Lb/2;
      while (t < stopIndex) {
	signalVector::const_iterator aP = aStart+t;
	signalVector::const_iterator aPsym = aP-Lb;
	signalVector::const_iterator bP = bStart;
	sum = 0.0;
	while (bP < bEnd) {
	  if (aP < aStart) break;
	  if (aP == aPsym)
	    sum+= (*aP)*(*bP);
	  else if ((aP < aEnd) && (aPsym >= aStart)) 
	    sum+= ((*aP)+(*aPsym))*(*bP);
	  else if (aP < aEnd)
	    sum += (*aP)*(*bP);
	  else if (aPsym >= aStart)
	    sum += (*aPsym)*(*bP);
	  aP--;
	  aPsym++;
	  bP++;
	}
	*cPtr++ = sum;
	t++;
      }
    }
    break;
  default:
    return NULL;
    break;
  }
    
    
  return c;
}


signalVector* generateGSMPulse(int symbolLength,
			       int samplesPerSymbol)
{

  int numSamples = samplesPerSymbol*symbolLength + 1;
  signalVector *x = new signalVector(numSamples);
  signalVector::iterator xP = x->begin();
  int centerPoint = (numSamples-1)/2;
  for (int i = 0; i < numSamples; i++) {
    float arg = (float) (i-centerPoint)/(float) samplesPerSymbol;
    *xP++ = 0.96*exp(-1.1380*arg*arg-0.527*arg*arg*arg*arg); // GSM pulse approx.
  }

  float avgAbsval = sqrtf(vectorNorm2(*x)/samplesPerSymbol);
  xP = x->begin();
  for (int i = 0; i < numSamples; i++) 
    *xP++ /= avgAbsval;
  x->isRealOnly(true);
  return x;
}

signalVector* frequencyShift(signalVector *y,
			     signalVector *x,
			     float freq,
			     float startPhase,
			     float *finalPhase)
{

  if (!x) return NULL;
 
  if (y==NULL) {
    y = new signalVector(x->size());
    y->isRealOnly(x->isRealOnly());
    if (y==NULL) return NULL;
  }

  if (y->size() < x->size()) return NULL;

  float phase = startPhase;
  signalVector::iterator yP = y->begin();
  signalVector::iterator xPEnd = x->end();
  signalVector::iterator xP = x->begin();

  if (x->isRealOnly()) {
    while (xP < xPEnd) {
      (*yP++) = expjLookup(phase)*( (xP++)->real() );
      phase += freq;
    }
  }
  else {
    while (xP < xPEnd) {
      (*yP++) = (*xP++)*expjLookup(phase);
      phase += freq;
    }
  }


  if (finalPhase) *finalPhase = phase;

  return y;
}


signalVector* correlate(signalVector *a,
			signalVector *b,
			signalVector *c,
			ConvType spanType)
{

  signalVector *tmp = new signalVector(b->size());
  tmp->isRealOnly(b->isRealOnly());
  signalVector::iterator bP = b->begin();
  signalVector::iterator bPEnd = b->end();
  signalVector::iterator tmpP = tmp->end()-1;
  if (!b->isRealOnly()) {
    while (bP < bPEnd) {
      *tmpP-- = bP->conj();
      bP++;
    }
  }
  else {
    while (bP < bPEnd) {
      *tmpP-- = bP->real();
      bP++;
    }
  }

  c = convolve(a,tmp,c,spanType);

  delete tmp;

  return c;
}


/* soft output slicer */
bool vectorSlicer(signalVector *x) 
{

  signalVector::iterator xP = x->begin();
  signalVector::iterator xPEnd = x->end();
  while (xP < xPEnd) {
    *xP = (complex) (0.5*(xP->real()+1.0F));
    if (xP->real() > 1.0) *xP = 1.0;
    if (xP->real() < 0.0) *xP = 0.0;
    xP++;
  }
  return true;
}
  
signalVector *modulateBurst(const BitVector &wBurst,
			    const signalVector &gsmPulse,
			    int guardPeriodLength,
			    int samplesPerSymbol)
{

  int burstSize = samplesPerSymbol*(wBurst.size()+guardPeriodLength);
  signalVector *modBurst = new signalVector(burstSize);
  modBurst->isRealOnly(true);
  modBurst->fill(0.0);
  signalVector::iterator modBurstItr = modBurst->begin();

#if 0 
  // if wBurst is already differentially decoded
  *modBurstItr = 2.0*(wBurst[0] & 0x01)-1.0;
  signalVector::iterator prevVal = modBurstItr;
  for (unsigned int i = 1; i < wBurst.size(); i++) {
    modBurstItr += samplesPerSymbol;
    if (wBurst[i] & 0x01) 
      *modBurstItr = *prevVal * complex(0.0,1.0);
    else
      *modBurstItr = *prevVal * complex(0.0,-1.0);
    prevVal = modBurstItr;
  }
#else
  // if wBurst are the raw bits
  for (unsigned int i = 0; i < wBurst.size(); i++) {
    *modBurstItr = 2.0*(wBurst[i] & 0x01)-1.0;
    modBurstItr += samplesPerSymbol;
  }

  // shift up pi/2
  // ignore starting phase, since spec allows for discontinuous phase
  GMSKRotate(*modBurst);
#endif
  modBurst->isRealOnly(false);

  // filter w/ pulse shape
  signalVector *shapedBurst = convolve(modBurst,&gsmPulse,NULL,NO_DELAY);

  delete modBurst;
  
  return shapedBurst;

}

float sinc(float x) 
{
  if ((x >= 0.01F) || (x <= -0.01F)) return (sinLookup(x)/x);
  return 1.0F;
}

void delayVector(signalVector &wBurst,
		 float delay)
{
  
  int   intOffset = (int) floor(delay);
  float fracOffset = delay - intOffset;
  signalVector *shiftedBurst = NULL;

  // do fractional shift first, only do it for reasonable offsets
  if (fabs(fracOffset) > 1e-2) {
    // create sinc function
    signalVector *sincVector = new signalVector(21);
    sincVector->isRealOnly(true);
    signalVector::iterator sincBurstItr = sincVector->begin();
    for (int i = 0; i < 21; i++) 
      *sincBurstItr++ = (complex) sinc(M_PI_F*(i-10-fracOffset));
  
    shiftedBurst = convolve(&wBurst,sincVector,shiftedBurst,NO_DELAY);

    delete sincVector;
  }
  else
    shiftedBurst = &wBurst;

  if (intOffset < 0) {
    intOffset = -intOffset;
    signalVector::iterator wBurstItr = wBurst.begin();
    signalVector::iterator shiftedItr = shiftedBurst->begin()+intOffset;
    while (shiftedItr < shiftedBurst->end())
      *wBurstItr++ = *shiftedItr++;
    while (wBurstItr < wBurst.end())
      *wBurstItr++ = 0.0;
  }
  else {
    signalVector::iterator wBurstItr = wBurst.end()-1;
    signalVector::iterator shiftedItr = shiftedBurst->end()-1-intOffset;
    while (shiftedItr >= shiftedBurst->begin())
      *wBurstItr-- = *shiftedItr--;
    while (wBurstItr >= wBurst.begin())
      *wBurstItr-- = 0.0;
  }

  if (shiftedBurst != &wBurst) delete shiftedBurst;
}
  
signalVector *gaussianNoise(int length, 
			    float variance, 
			    complex mean)
{

  signalVector *noise = new signalVector(length);
  signalVector::iterator nPtr = noise->begin();
  float stddev = sqrtf(variance);
  while (nPtr < noise->end()) {
    float u1 = (float) rand()/ (float) RAND_MAX;
    while (u1==0.0)
      u1 = (float) rand()/ (float) RAND_MAX;
    float u2 = (float) rand()/ (float) RAND_MAX;
    float arg = 2.0*M_PI*u2;
    *nPtr = mean + stddev*complex(cos(arg),sin(arg))*sqrtf(-2.0*log(u1));
    nPtr++;
  }

  return noise;
}

complex interpolatePoint(const signalVector &inSig,
			 float ix)
{
  
  int start = (int) (floor(ix) - 10);
  if (start < 0) start = 0;
  int end = (int) (floor(ix) + 11);
  if ((unsigned) end > inSig.size()-1) end = inSig.size()-1;
  
  complex pVal = 0.0;
  if (!inSig.isRealOnly()) {
    for (int i = start; i < end; i++) 
      pVal += inSig[i] * sinc(M_PI_F*(i-ix));
  }
  else {
    for (int i = start; i < end; i++) 
      pVal += inSig[i].real() * sinc(M_PI_F*(i-ix));
  }
   
  return pVal;
}

  
 
complex peakDetect(const signalVector &rxBurst,
		   float *peakIndex,
		   float *avgPwr) 
{
  

  complex maxVal = 0.0;
  float maxIndex = -1;
  float sumPower = 0.0;

  for (unsigned int i = 0; i < rxBurst.size(); i++) {
    float samplePower = rxBurst[i].norm2();
    if (samplePower > maxVal.real()) {
      maxVal = samplePower;
      maxIndex = i;
    }
    sumPower += samplePower;
  }

  // interpolate around the peak
  // to save computation, we'll use early-late balancing
  float earlyIndex = maxIndex-1;
  float lateIndex = maxIndex+1;
  
  float incr = 0.5;
  while (incr > 1.0/1024.0) {
    complex earlyP = interpolatePoint(rxBurst,earlyIndex);
    complex lateP =  interpolatePoint(rxBurst,lateIndex);
    if (earlyP < lateP) 
      earlyIndex += incr;
    else if (earlyP > lateP)
      earlyIndex -= incr;
    else break;
    incr /= 2.0;
    lateIndex = earlyIndex + 2.0;
  }

  maxIndex = earlyIndex + 1.0;
  maxVal = interpolatePoint(rxBurst,maxIndex);

  if (peakIndex!=NULL)
    *peakIndex = maxIndex;

  if (avgPwr!=NULL)
    *avgPwr = (sumPower-maxVal.norm2()) / (rxBurst.size()-1);

  return maxVal;

}

void scaleVector(signalVector &x,
		 complex scale)
{
  signalVector::iterator xP = x.begin();
  signalVector::iterator xPEnd = x.end();
  if (!x.isRealOnly()) {
    while (xP < xPEnd) {
      *xP = *xP * scale;
      xP++;
    }
  }
  else {
    while (xP < xPEnd) {
      *xP = xP->real() * scale;
      xP++;
    }
  }
}

/** in-place conjugation */
void conjugateVector(signalVector &x)
{
  if (x.isRealOnly()) return;
  signalVector::iterator xP = x.begin();
  signalVector::iterator xPEnd = x.end();
  while (xP < xPEnd) {
    *xP = xP->conj();
    xP++;
  }
}


// in-place addition!!
bool addVector(signalVector &x,
	       signalVector &y)
{
  signalVector::iterator xP = x.begin();
  signalVector::iterator yP = y.begin();
  signalVector::iterator xPEnd = x.end();
  signalVector::iterator yPEnd = y.end();
  while ((xP < xPEnd) && (yP < yPEnd)) {
    *xP = *xP + *yP;
    xP++; yP++;
  }
  return true;
}

void offsetVector(signalVector &x,
		  complex offset)
{
  signalVector::iterator xP = x.begin();
  signalVector::iterator xPEnd = x.end();
  if (!x.isRealOnly()) {
    while (xP < xPEnd) {
      *xP += offset;
      xP++;
    }
  }
  else {
    while (xP < xPEnd) {
      *xP = xP->real() + offset;
      xP++;
    }      
  }
}

bool generateMidamble(signalVector &gsmPulse,
		      int samplesPerSymbol,
		      int TSC)
{

  if ((TSC < 0) || (TSC > 7)) 
    return false;

  if ((gMidambles[TSC]) && (gMidambles[TSC]->sequence!=NULL))
    delete gMidambles[TSC]->sequence;

  signalVector emptyPulse(1); 
  *(emptyPulse.begin()) = 1.0;

  // only use middle 16 bits of each TSC
  signalVector *middleMidamble = modulateBurst(gTrainingSequence[TSC].segment(5,16),
					 emptyPulse,
					 0,
					 samplesPerSymbol);
  signalVector *midamble = modulateBurst(gTrainingSequence[TSC],
                                         gsmPulse,
                                         0,
                                         samplesPerSymbol);
  
  if (midamble == NULL) return false;
  if (middleMidamble == NULL) return false;

  // NOTE: Because ideal TSC 16-bit midamble is 66 symbols into burst,
  //       the ideal TSC has an + 180 degree phase shift,
  //       due to the pi/2 frequency shift, that 
  //       needs to be accounted for.
  //       26-midamble is 61 symbols into burst, has +90 degree phase shift.
  scaleVector(*middleMidamble,complex(-1.0,0.0));
  scaleVector(*midamble,complex(0.0,1.0));

  signalVector *autocorr = correlate(midamble,middleMidamble,NULL,NO_DELAY);
  
  if (autocorr == NULL) return false;

  gMidambles[TSC] = new CorrelationSequence;
  gMidambles[TSC]->sequence = middleMidamble;

  gMidambles[TSC]->gain = peakDetect(*autocorr,&gMidambles[TSC]->TOA,NULL);
  gMidambles[TSC]->TOA -= 5*samplesPerSymbol;

  delete autocorr;
  delete midamble;

  return true;
}

bool generateRACHSequence(signalVector &gsmPulse,
			  int samplesPerSymbol)
{
  
  if ((gRACHSequence) && (gRACHSequence->sequence!=NULL))
    delete gRACHSequence->sequence;

  signalVector *RACHSeq = modulateBurst(gRACHSynchSequence,
					gsmPulse,
					0,
					samplesPerSymbol);

  assert(RACHSeq);

  signalVector *autocorr = correlate(RACHSeq,RACHSeq,NULL,NO_DELAY);

  assert(autocorr);

  gRACHSequence = new CorrelationSequence;
  gRACHSequence->sequence = RACHSeq;
  
  gRACHSequence->gain = peakDetect(*autocorr,&gRACHSequence->TOA,NULL);
 
  delete autocorr;

  return true;

}

				
bool detectRACHBurst(signalVector &rxBurst,
		     float detectThreshold,
		     int samplesPerSymbol,
		     complex *amplitude,
		     float* TOA)
{
  
  signalVector *correlatedRACH = correlate(&rxBurst,gRACHSequence->sequence,
					   NULL,
					   NO_DELAY);
  assert(correlatedRACH);

  float meanPower;
  complex peakAmpl = peakDetect(*correlatedRACH,TOA,&meanPower);

  float valleyPower = 0.0; 
 
  // check for bogus results
  if ((*TOA < 0.0) || (*TOA > correlatedRACH->size())) {
	delete correlatedRACH;
        *amplitude = 0.0;
	return false;
  }
  complex *peakPtr = correlatedRACH->begin() + (int) rint(*TOA);

  DCOUT("RACH corr: " << *correlatedRACH);

  float numSamples = 0.0;
  for (int i = 57*samplesPerSymbol; i <= 107*samplesPerSymbol;i++) {
    if (peakPtr+i >= correlatedRACH->end())
      break;
    valleyPower += (peakPtr+i)->norm2();
    numSamples++;
  }

  if (numSamples < 2) {
	delete correlatedRACH;
        *amplitude = 0.0;
        return false;
  }

  float RMS = sqrtf(valleyPower/(float) numSamples)+0.00001;
  float peakToMean = peakAmpl.abs()/RMS;

  COUT("RACH peakAmpl=" << peakAmpl << " RMS=" << RMS << " peakToMean=" << peakToMean);
  *amplitude = peakAmpl/(gRACHSequence->gain);

  *TOA = (*TOA) - gRACHSequence->TOA - 8*samplesPerSymbol;

  delete correlatedRACH;

  COUT("RACH thresh: " << peakToMean);

  return (peakToMean > detectThreshold);
}

bool energyDetect(signalVector &rxBurst,
		  unsigned windowLength,
		  float detectThreshold,
                  float *avgPwr)
{

  signalVector::const_iterator windowItr = rxBurst.begin(); //+rxBurst.size()/2 - 5*windowLength/2;
  float energy = 0.0;
  if (windowLength < 0) windowLength = 20;
  if (windowLength > rxBurst.size()) windowLength = rxBurst.size();
  for (unsigned i = 0; i < windowLength; i++) {
    energy += windowItr->norm2();
    windowItr++;
  }
  if (avgPwr) *avgPwr = energy/windowLength;
  DCOUT("detected energy: " << energy/windowLength);
  return (energy/windowLength > detectThreshold*detectThreshold);
}
  

bool analyzeTrafficBurst(signalVector &rxBurst,
			 unsigned TSC,
			 float detectThreshold,
			 int samplesPerSymbol,
			 complex *amplitude,
			 float *TOA,
			 bool requestChannel,
                         signalVector **channelResponse,
			 float *channelResponseOffset) 
{

  assert(TSC<8);
  assert(amplitude);
  assert(TOA);
  assert(gMidambles[TSC]);

  signalVector burstSegment(rxBurst.begin(),samplesPerSymbol*56,36*samplesPerSymbol);

  signalVector *correlatedBurst = correlate(&burstSegment, //&rxBurst,
					    gMidambles[TSC]->sequence,
					    NULL,NO_DELAY);
  assert(correlatedBurst);

  float meanPower;
  *amplitude = peakDetect(*correlatedBurst,TOA,&meanPower);
  float valleyPower = 0.0; //amplitude->norm2();
  complex *peakPtr = correlatedBurst->begin() + (int) rint(*TOA);

  // check for bogus results
  if ((*TOA < 0.0) || (*TOA > correlatedBurst->size())) {
        delete correlatedBurst;
        *amplitude = 0.0;
        return false;
  }

  int numRms = 0;
  for (int i = 2*samplesPerSymbol; i <= 5*samplesPerSymbol;i++) {
    if (peakPtr - i >= correlatedBurst->begin()) { 
      valleyPower += (peakPtr-i)->norm2();
      numRms++;
    }
    if (peakPtr + i < correlatedBurst->end()) {
      valleyPower += (peakPtr+i)->norm2();
      numRms++;
    }
  }

  if (numRms < 2) {
        // check for bogus results
        delete correlatedBurst;
        *amplitude = 0.0;
        return false;
  }

  float RMS = sqrtf(valleyPower/(float)numRms)+0.00001;
  float peakToMean = (amplitude->abs())/RMS;

  // NOTE: Because ideal TSC is 66 symbols into burst,
  //       the ideal TSC has an +/- 180 degree phase shift,
  //       due to the pi/4 frequency shift, that 
  //       needs to be accounted for.
  
  *amplitude = (*amplitude)/gMidambles[TSC]->gain;
  *TOA = (*TOA)-gMidambles[TSC]->TOA;
 
  (*TOA) = (*TOA) - (66-56)*samplesPerSymbol;
  COUT("TCH peakAmpl=" << amplitude->abs() << " RMS=" << RMS << " peakToMean=" << peakToMean << " TOA=" << *TOA);

  DCOUT("autocorr: " << *correlatedBurst);
  
  if (requestChannel && (peakToMean > detectThreshold)) {
    float TOAoffset = gMidambles[TSC]->TOA+(66-56)*samplesPerSymbol;
    delayVector(*correlatedBurst,-(*TOA));
    // midamble only allows estimation of a 6-tap channel
    signalVector channelVector(6*samplesPerSymbol);
    float maxEnergy = -1.0;
    int maxI = -1;
    for (int i = 0; i < 7; i++) {
      if (TOAoffset+(i-5)*samplesPerSymbol + channelVector.size() > correlatedBurst->size()) continue;
      if (TOAoffset+(i-5)*samplesPerSymbol < 0) continue;
      correlatedBurst->segmentCopyTo(channelVector,(int) floor(TOAoffset+(i-5)*samplesPerSymbol),channelVector.size());
      float energy = vectorNorm2(channelVector);
      if (energy > 0.95*maxEnergy) {
	maxI = i;
	maxEnergy = energy;
      }
    }
	
    *channelResponse = new signalVector(channelVector.size());
    correlatedBurst->segmentCopyTo(**channelResponse,(int) floor(TOAoffset+(maxI-5)*samplesPerSymbol),(*channelResponse)->size());
    scaleVector(**channelResponse,complex(1.0,0.0)/gMidambles[TSC]->gain);
    COUT("channelResponse: " << **channelResponse);
    
    if (channelResponseOffset) 
      *channelResponseOffset = 5*samplesPerSymbol-maxI;
      
  }

  delete correlatedBurst;

  return (peakToMean > detectThreshold);
		  
}

signalVector *decimateVector(signalVector &wVector,
			     int decimationFactor) 
{
  
  if (decimationFactor <= 1) return NULL;

  signalVector *decVector = new signalVector(wVector.size()/decimationFactor);
  decVector->isRealOnly(wVector.isRealOnly());

  signalVector::iterator vecItr = decVector->begin();
  for (unsigned int i = 0; i < wVector.size();i+=decimationFactor) 
    *vecItr++ = wVector[i];

  return decVector;
}


SoftVector *demodulateBurst(const signalVector &rxBurst,
			 const signalVector &gsmPulse,
			 int samplesPerSymbol,
			 complex channel,
			 float TOA) 

{

  signalVector *demodBurst = new signalVector(rxBurst);

  scaleVector(*demodBurst,((complex) 1.0)/channel);

  delayVector(*demodBurst,-TOA);
  signalVector *shapedBurst = demodBurst;

  // shift up by a quarter of a frequency
  // ignore starting phase, since spec allows for discontinuous phase
  GMSKReverseRotate(*shapedBurst);

  // run through slicer
  if (samplesPerSymbol > 1) {
     signalVector *decShapedBurst = decimateVector(*shapedBurst,samplesPerSymbol);
     delete shapedBurst;
     shapedBurst = decShapedBurst;
  }

  DCOUT("shapedBurst: " << *shapedBurst);

  vectorSlicer(shapedBurst);

  SoftVector *burstBits = new SoftVector(shapedBurst->size());

  SoftVector::iterator burstItr = burstBits->begin();
  signalVector::iterator shapedItr = shapedBurst->begin();
  for (; shapedItr < shapedBurst->end(); shapedItr++) 
    *burstItr++ = shapedItr->real();

  delete shapedBurst;

  return burstBits;

}


// 1.0 is sampling frequency
// must satisfy cutoffFreq > 1/filterLen
signalVector *createLPF(float cutoffFreq,
			int filterLen,
			float gainDC)
{
  /*
  signalVector *LPF = new signalVector(filterLen);
  LPF->isRealOnly(true);
  signalVector::iterator itr = LPF->begin();
  double sum = 0.0;
  for (int i = 0; i < filterLen; i++) {
    float ys = sinc(M_2PI_F*cutoffFreq*((float)i-(float)(filterLen+1)/2.0F));
    float yg = 4.0F * cutoffFreq;
    float yw = 0.53836F - 0.46164F * cos(((float)i)*M_2PI_F/(float)(filterLen+1));
    *itr++ = (complex) ys*yg*yw;
    sum += ys*yg*yw;
  }
  */
  double sum = 0.0;
  signalVector *LPF;
  signalVector::iterator itr;
  if (filterLen == 651) { // receive LPF
    LPF = new signalVector(651);
    LPF->isRealOnly(true);
    itr = LPF->begin();
    for (int i = 0; i < filterLen; i++) {
       *itr++ = complex(rcvLPF_651[i],0.0);
       sum += rcvLPF_651[i];
    }
  }
  else { 
    LPF = new signalVector(961);
    LPF->isRealOnly(true);
    itr = LPF->begin();
    for (int i = 0; i < filterLen; i++) {
       *itr++ = complex(sendLPF_961[i],0.0);
       sum += sendLPF_961[i];
    }
  }

  float normFactor = gainDC/sum; //sqrtf(gainDC/vectorNorm2(*LPF));
  // normalize power
  itr = LPF->begin();
  for (int i = 0; i < filterLen; i++) {
    *itr = *itr*normFactor;
    itr++;
  }
  return LPF;

}
    


#define POLYPHASESPAN 10

// assumes filter group delay is 0.5*(length of filter)
signalVector *polyphaseResampleVector(signalVector &wVector,
				      int P, int Q,
				      signalVector *LPF)

{
 
  bool deleteLPF = false;
 
  if (LPF==NULL) {
    float cutoffFreq = (P < Q) ? (1.0/(float) Q) : (1.0/(float) P);
    LPF = createLPF(cutoffFreq/3.0,100*POLYPHASESPAN+1,Q);
    deleteLPF = true;
  }

  signalVector *resampledVector = new signalVector((int) ceil(wVector.size()*(float) P / (float) Q));
  resampledVector->fill(0);
  resampledVector->isRealOnly(wVector.isRealOnly());
  signalVector::iterator newItr = resampledVector->begin();

  //FIXME: need to update for real-only vectors
  int outputIx = (LPF->size()-1)/2/Q; //((P > Q) ? P : Q); 
  while (newItr < resampledVector->end()) {
    int outputBranch = (outputIx*Q) % P; 
    int inputOffset = (outputIx*Q - outputBranch)/P;
    signalVector::const_iterator inputItr = wVector.begin() + inputOffset;
    signalVector::const_iterator filtItr  = LPF->begin() + outputBranch;
    while (inputItr >= wVector.end()) {
      inputItr--;
      filtItr+=P;
    }
    complex sum = 0.0;
    if (!LPF->isRealOnly()) {
      while ( (inputItr >= wVector.begin()) && (filtItr < LPF->end()) ) {
	sum += (*inputItr)*(*filtItr);
	inputItr--;
	filtItr += P;
      }
    }
    else {
      while ( (inputItr >= wVector.begin()) && (filtItr < LPF->end()) ) {
	sum += (*inputItr)*(filtItr->real());
	inputItr--;
	filtItr += P;
      }
    }
    *newItr = sum;
    newItr++;
    outputIx++;
  }
      
  if (deleteLPF) delete LPF;

  return resampledVector;
}


signalVector *resampleVector(signalVector &wVector,
			     float expFactor,
			     complex endPoint)

{

  if (expFactor < 1.0) return NULL;

  signalVector *retVec = new signalVector((int) ceil(wVector.size()*expFactor));

  float t = 0.0;
  
  signalVector::iterator retItr = retVec->begin();
  while (retItr < retVec->end()) {
    unsigned tLow = (unsigned int) floor(t);
    unsigned tHigh = tLow + 1;
    if (tLow > wVector.size()-1) break;
    if (tHigh > wVector.size()) break;
    complex lowPoint = wVector[tLow];
    complex highPoint = (tHigh == wVector.size()) ? endPoint : wVector[tHigh];
    complex a = (tHigh-t);
    complex b = (t-tLow);
    *retItr = (a*lowPoint + b*highPoint);
    t += 1.0/expFactor;
  }

  return retVec;

}
		   

// Assumes symbol-spaced sampling!!!
// Based upon paper by Al-Dhahir and Cioffi
bool designDFE(signalVector &channelResponse,
	       float SNRestimate,
	       int Nf,
	       signalVector **feedForwardFilter,
	       signalVector **feedbackFilter)
{
  
  signalVector G0(Nf);
  signalVector G1(Nf);
  signalVector::iterator G0ptr = G0.begin();
  signalVector::iterator G1ptr = G1.begin();
  signalVector::iterator chanPtr = channelResponse.begin();

  int nu = channelResponse.size()-1;

  *G0ptr = 1.0/sqrtf(SNRestimate);
  for(int j = 0; j <= nu; j++) {
    *G1ptr = chanPtr->conj();
    G1ptr++; chanPtr++;
  }

  signalVector *L[Nf];
  signalVector::iterator Lptr;
  float d;
  for(int i = 0; i < Nf; i++) {
    d = G0.begin()->norm2() + G1.begin()->norm2();
    L[i] = new signalVector(Nf+nu);
    Lptr = L[i]->begin()+i;
    G0ptr = G0.begin(); G1ptr = G1.begin();
    while ((G0ptr < G0.end()) &&  (Lptr < L[i]->end())) {
      *Lptr = (*G0ptr*(G0.begin()->conj()) + *G1ptr*(G1.begin()->conj()) )/d;
      Lptr++;
      G0ptr++;
      G1ptr++;
    }
    complex k = (*G1.begin())/(*G0.begin());

    if (i != Nf-1) {
      signalVector G0new = G1;
      scaleVector(G0new,k.conj());
      addVector(G0new,G0);

      signalVector G1new = G0;
      scaleVector(G1new,k*(-1.0));
      addVector(G1new,G1);
      delayVector(G1new,-1.0);

      scaleVector(G0new,1.0/sqrtf(1.0+k.norm2()));
      scaleVector(G1new,1.0/sqrtf(1.0+k.norm2()));
      G0 = G0new;
      G1 = G1new;
    }
  }

  *feedbackFilter = new signalVector(nu);
  L[Nf-1]->segmentCopyTo(**feedbackFilter,Nf,nu);
  scaleVector(**feedbackFilter,(complex) -1.0);
  conjugateVector(**feedbackFilter);

  signalVector v(Nf);
  signalVector::iterator vStart = v.begin();
  signalVector::iterator vPtr;
  *(vStart+Nf-1) = (complex) 1.0;
  for(int k = Nf-2; k >= 0; k--) {
    Lptr = L[k]->begin()+k+1;
    vPtr = vStart + k+1;
    complex v_k = 0.0;
    for (int j = k+1; j < Nf; j++) {
      v_k -= (*vPtr)*(*Lptr);
      vPtr++; Lptr++;
    }
     *(vStart + k) = v_k;
  }

  *feedForwardFilter = new signalVector(Nf);
  signalVector::iterator w = (*feedForwardFilter)->begin();
  for (int i = 0; i < Nf; i++) {
    delete L[i];
    complex w_i = 0.0;
    int endPt = ( nu < (Nf-1-i) ) ? nu : (Nf-1-i);
    vPtr = vStart+i;
    chanPtr = channelResponse.begin();
    for (int k = 0; k < endPt+1; k++) {
      w_i += (*vPtr)*(chanPtr->conj());
      vPtr++; chanPtr++;
    }
    *w = w_i/d;
    w++;
  }


  return true;
  
}

// Assumes symbol-rate sampling!!!!
SoftVector *equalizeBurst(signalVector &rxBurst,
		       float TOA,
		       int samplesPerSymbol,
		       signalVector &w, // feedforward filter
		       signalVector &b) // feedback filter
{

  delayVector(rxBurst,-TOA);

  signalVector* postForwardFull = convolve(&rxBurst,&w,NULL,FULL_SPAN);

  signalVector* postForward = new signalVector(rxBurst.size());
  postForwardFull->segmentCopyTo(*postForward,w.size()-1,rxBurst.size());
  delete postForwardFull;

  signalVector::iterator dPtr = postForward->begin();
  signalVector::iterator dBackPtr;
  signalVector::iterator rotPtr = GMSKRotation->begin();
  signalVector::iterator revRotPtr = GMSKReverseRotation->begin();

  signalVector *DFEoutput = new signalVector(postForward->size());
  signalVector::iterator DFEItr = DFEoutput->begin();

  // NOTE: can insert the midamble and/or use midamble to estimate BER
  for (; dPtr < postForward->end(); dPtr++) {
    dBackPtr = dPtr-1;
    signalVector::iterator bPtr = b.begin();
    while ( (bPtr < b.end()) && (dBackPtr >= postForward->begin()) ) {
      *dPtr = *dPtr + (*bPtr)*(*dBackPtr);
      bPtr++;
      dBackPtr--;
    }
    *dPtr = *dPtr * (*revRotPtr);
    *DFEItr = *dPtr;
    // make decision on symbol
    *dPtr = (dPtr->real() > 0.0) ? 1.0 : -1.0;
    //*DFEItr = *dPtr;
    *dPtr = *dPtr * (*rotPtr);
    DFEItr++;
    rotPtr++;
    revRotPtr++;
  }

  vectorSlicer(DFEoutput);

  SoftVector *burstBits = new SoftVector(postForward->size());
  SoftVector::iterator burstItr = burstBits->begin();
  DFEItr = DFEoutput->begin();
  for (; DFEItr < DFEoutput->end(); DFEItr++) 
    *burstItr++ = DFEItr->real();

  delete postForward;

  delete DFEoutput;

  return burstBits;
}
