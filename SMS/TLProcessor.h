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



#ifndef TL_PROCESSOR_H
#define TL_PROCESSOR_H

namespace SMS {

// forward declarations.
class TLFrame;
class RLFrame;
class TLMessage;
class RLProcessor;



class TLMessageFIFO : public InterthreadFIFO<TLMessage>{};

class TLProcessor 
{
	public:
	TLMessageFIFO mUplinkFIFO;		
	RLProcessor* mDownstream;		

	void TLProcessor::writeHighSide( const TLMessage& msg, const SMSPrimitive& prim );
	void writeHighSide( const SMSPrimitive& prim );
	void writeLowSide( const TLFrame& );	
	void downstream ( RLProcessor* wDownstream ) {mDownstream=wDownstream;}

	void receiveData( const TLFrame& );
	
	TLProcessor(): mDownstream(NULL){}
	~TLProcessor(){}

};




}; // namespace SMS {

#endif
