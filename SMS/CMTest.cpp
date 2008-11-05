/*
* Copyright (c) 2008, Kestrel Signal Processing, Inc.
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


#include <stdio.h>
#include <iostream>
#include "CommSig.h"


#include "GSMTransfer.h"
#include "ControlCommon.h"
#include "GSMConfig.h"
#include "SIPInterface.h"
#include "GSMLogicalChannel.h"



#include "SMSTransfer.h"

#include "CMMessage.h"
#include "RLMessage.h"
#include "TLMessage.h"

#include "CMProcessor.h"
#include "RLProcessor.h"
#include "TLProcessor.h"

using namespace std;
using namespace GSM;
using namespace SMS;
using namespace Control;
using namespace SIP;

#define DEBUG 1

GSMConfig gBTS(0,0);
TransactionTable gTransactionTable;
SIPInterface gSIPInterface(5050, "0.0.0.0", 5060);



int main() {  

	SDCCHLogicalChannel_LB sdcch(0, gSDCCH4_0, gBTS);
	CMProcessor cm_proc;
	RLProcessor rl_proc;
	TLProcessor tl_proc;

	sdcch.open();	

	cm_proc.TIFlag(0);
	cm_proc.TIValue(3);
	cm_proc.downstream(&sdcch);	
	cm_proc.upstream(&rl_proc);
	cm_proc.open();

	rl_proc.downstream(&cm_proc);
	rl_proc.upstream(&tl_proc);
	rl_proc.open();

	tl_proc.downstream(&rl_proc);
//	tl_proc.open();
	sleep(1);


	// 0
	// ms tx - mobile originated message.
	L3Frame cp_data("00111001000000010001100100000000000000010000000000000111100100010011000100100001000100111001010000011000111100000000110100010001000000000000001110000001001000011111001100000000000000001111111100000011001011100111000100011001");
	L3Message * msg =parseL3(cp_data);	
	sdcch.sendL3(*msg, DATA_INDICATION, 0);		
	sleep(1);
	
	// 1
	
	TLMessage * sms =  tl_proc.mUplinkFIFO.read();
	DCOUT("msg = "<<*sms)
	sleep(1);
	DCOUT(" writing Submit Report ")	
	tl_proc.writeHighSide( SubmitReport("08162312394401"), SM_RL_REPORT_REQ );
	sleep(1);


	CPAck ack;
	sdcch.sendL3(ack, DATA_INDICATION, 0);	
	sleep(1);	



	printf(" Hello World\n");
	while(1){ sleep(1); }
	return 0; 
}


