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

/*
Contributors:
Raffi Sevlian, raffisev@gmail.com
*/


#ifndef RL_PROCESSOR_H
#define RL_PROCESSOR_H


namespace SMS {

// forward declarations.
class TLFrame;
class RLFrame;
class RPMessage;
class CMProcessor;
class TLProcessor;


class RLProcessor 
{
	public:
	enum RLState {
		Idle=0,
		MT_WaitForRPAck=1,
		MO_WaitToSendRPAck=2
	};

	RLState mState;
	CMProcessor* mDownstream;	///< downstream CMProcessor.
	TLProcessor* mUpstream;		
	

	volatile bool mActive;
	volatile bool mRunning;

	void writeHighSide( const TLFrame& txframe );
	void writeLowSide( const RLFrame& rxframe );
	
	RLProcessor()
		:mState(Idle), mDownstream(NULL), mUpstream(NULL)
	{ }
	
	void open();
	void close();
	void stop();
	void start();
	bool active() { return mActive; }
	void downstream ( CMProcessor* wDownstream ){ mDownstream = wDownstream; }
	void upstream(TLProcessor *wUpstream) { mUpstream=wUpstream; }

	void writeTLFrame( const TLFrame& frame );
	void writeRPMessage( const RPMessage& msg, const SMSPrimitive& prim );

	public:
	friend void * RLProcessorUpstreamServiceLoopAdapter(RLProcessor*);

};


void * RLProcessorUpstreamServiceLoopAdapter(RLProcessor*);
std::ostream& operator<<(std::ostream& os, RLProcessor::RLState );


}; // namespace SMS {

#endif
