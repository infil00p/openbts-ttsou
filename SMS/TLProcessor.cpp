/*
* Copyright 2008 Free Software Foundation, Inc.
*

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

* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*/



#include <stdio.h>

#include "Common.h"
#include "GSMCommon.h"
#include "GSMCommon.h"
#include "GSMLogicalChannel.h"
#include "ControlCommon.h"

#include "SMSTransfer.h"
#include "CMMessage.h"
#include "RLMessage.h"

#include "TLMessage.h"
#include "RLProcessor.h"
#include "TLProcessor.h"

using namespace std;
using namespace SMS;

#define DEBUG 1


void TLProcessor::writeLowSide( const TLFrame& frame ) 
{
	DCOUT("TLProcessor::writeLowSide frame="<<frame)
	if( frame.primitive() == SM_RL_DATA_IND ){
		TLMessage * msg = parseTL(frame);
		DCOUT("TLProcessor::writeLowSide TLMessage=  "<<*msg)
		if( msg != NULL )
			mUplinkFIFO.write(msg);
	}	
	
}

void TLProcessor::receiveData( const TLFrame& frame )
{ }



void TLProcessor::writeHighSide( const TLMessage& msg, const SMSPrimitive& prim )
{ 
	DCOUT("TLProcessor::writeHighSide msg="<<msg)
	if(mDownstream==NULL){ 
		CERR("WARNING -- TLProcessor::writeHighSide no RLProcessor ") 
		return ;
	}
	TLFrame frame(prim, msg.bitsNeeded());
	msg.write(frame);
	mDownstream->writeHighSide(frame);
}

void TLProcessor::writeHighSide(const SMSPrimitive& prim )
{
	if(mDownstream==NULL){ 
		CERR("WARNING -- TLProcessor::writeHighSide no RLProcessor ") 
		return ;
	}
	TLFrame frame(prim);
	mDownstream->writeHighSide(frame);
}



