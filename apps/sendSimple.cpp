/*
* Copyright 2008 Free Software Foundation, Inc.
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


#include <stdio.h>
#include <iostream>
#include <Sockets.h>
#include <Threads.h>


UDPSocket sock(5063,"127.0.0.1",5062);


void *readerTask(void*)
{
	char buffer[2048];
	while (1) {
		sock.read(buffer);
		std::cout << "RECEIVED:\n" << buffer << std::endl;
	}
}


int main(int argc, char *argv[])
{

	Thread readerThread;
	readerThread.start(readerTask,NULL);

	char *IMSI = argv[1];
	const char form[] = "MESSAGE sip:%s@localhost SIP/2.0\nVia: SIP/2.0/TCP localhost;branch=z9hG4bK776sgdkse\nMax-Forwards: 2\nFrom: 1234567890@localhost:5063;tag=49583\nTo: sip:%s@localhost\nCall-ID: asd88asd77a@127.0.0.1:5063\nCSeq: 1 MESSAGE\nContent-Type: text/plain\nContent-Length: %d\n\n%s\n";
	char inbuf[1024];
	char outbuf[2048];
	while (1) {
		std::cin.getline(inbuf,1024,'\n');
		sprintf(outbuf,form,IMSI,IMSI,strlen(inbuf),inbuf);
		std::cout << "SENDING:\n" << outbuf << std::endl;
		sock.write(outbuf);
	}
}
