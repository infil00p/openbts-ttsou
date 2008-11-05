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
David A. Burgess, dburgess@ketsrelsp.com
*/

#ifndef SOCKETS_H
#define SOCKETS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <list>
#include "Assert.h"





#define MAX_UDP_LENGTH 1500


/** An exception to throw when a critical socket operation fails. */
class SocketError {};
#define SOCKET_ERROR {throw SocketError(); }

/** Abstract class for connectionless sockets. */
class DatagramSocket {

protected:

	int mSocketFD;				///< underlying file descriptor
	char mDestination[256];		///< address to which packets are sent
	char mReturnAddr[256];		///< return address of most recent received packet

public:

	/** An almost-does-nothing constructor. */
	DatagramSocket();

	virtual ~DatagramSocket();

	/** Return the address structure size for this socket type. */
	virtual size_t addressSize() const = 0;

	/**
		Send a binary packet.
		@param buffer The data bytes to send.
		@param length Number of bytes to send, or strlen(buffer) if defaulted to -1.
		@return number of bytes written, or -1 on error.
	*/
	int write( const char * buffer, size_t length);

	/**
		Send a C-style string packet.
		@param buffer The data bytes to send.
		@return number of bytes written, or -1 on error.
	*/
	int write( const char * buffer);

	/**
		Receive a packet.
		@param buffer A char[MAX_UDP_LENGTH] procured by the caller.
		@return The number of bytes received or -1 on non-blocking pass.
	*/
	int read(char* buffer);

	/** Make the socket non-blocking. */
	void nonblocking();

	/** Close the socket. */
	void close();

};



/** UDP/IP User Datagram Socket */
class UDPSocket : public DatagramSocket {

public:

	/** Open a USP socket with an OS-assigned port and no default destination. */
	UDPSocket( unsigned short localPort=0);

	/** Given a full specification, open the socket and set the dest address. */
	UDPSocket( 	unsigned short localPort, 
			const char * remoteIP, unsigned short remotePort);

	/** Set the destination port. */
	void destination( unsigned short wDestPort, const char * wDestIP );

	/** Return the actual port number in use. */
	unsigned short port() const;

	/** Open and bind the UDP socket to a local port. */
	void open(unsigned short localPort=0);

protected:

	size_t addressSize() const { return sizeof(struct sockaddr_in); }

};


/** Unix Domain Datagram Socket */
class UDDSocket : public DatagramSocket {

public:

	UDDSocket(const char* localPath=NULL, const char* remotePath=NULL);

	void destination(const char* remotePath);

	void open(const char* localPath);

protected:

	size_t addressSize() const { return sizeof(struct sockaddr_un); }

};


#endif



// vim: ts=4 sw=4
