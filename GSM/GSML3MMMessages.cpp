/**@file
	@brief GSM Mobility Management messages, from GSM 04.08 9.2.
*/

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
Raffi Sevlian, raffisev@gmail.com
*/



#include <iostream>

#include "GSML3CommonElements.h"
#include "GSML3MMMessages.h"


using namespace std;
using namespace GSM;



ostream& GSM::operator<<(ostream& os, L3MMMessage::MessageType val)
{
	switch (val) {
		case L3MMMessage::IMSIDetachIndication: 
			os << "IMSI Detach Indication"; break;
		case L3MMMessage::CMServiceRequest: 
			os << "CM Service Request"; break;
		case L3MMMessage::CMServiceAccept: 
			os << "CM Service Accept"; break;
		case L3MMMessage::CMServiceReject: 
			os << "CM Service Reject"; break;
		case L3MMMessage::CMServiceAbort: 
			os << "CM Service Abort"; break;
		case L3MMMessage::CMReestablishmentRequest:
			os << "CM Re-establishment Request"; break;
		case L3MMMessage::LocationUpdatingAccept: 
			os << "Location Updating Accept"; break;
		case L3MMMessage::LocationUpdatingReject: 
			os << "Location Updating Reject"; break;
		case L3MMMessage::LocationUpdatingRequest: 
			os << "Location Updating Request"; break;
		case L3MMMessage::IdentityResponse: 
			os << "Identity Response"; break;
		case L3MMMessage::MMStatus: 
			os << "MM Status"; break;
		default: os << hex << "0x" << (int)val << dec;
	}
	return os;
}




L3MMMessage* GSM::L3MMFactory(L3MMMessage::MessageType MTI)
{
	switch (MTI) {
	  case L3MMMessage::LocationUpdatingRequest: return new L3LocationUpdatingRequest;
	  case L3MMMessage::IMSIDetachIndication: return new L3IMSIDetachIndication;
	  case L3MMMessage::CMServiceRequest: return new L3CMServiceRequest;
	  case L3MMMessage::CMReestablishmentRequest: return new L3CMReestablishmentRequest;
	  case L3MMMessage::MMStatus: return new L3MMStatus;
	  case L3MMMessage::IdentityResponse: return new L3IdentityResponse;
	  default:
	    CERR("WARNING -- no L3 MM factory support for message "<<MTI);
		return NULL;
	}
}

L3MMMessage * GSM::parseL3MM(const L3Frame& source)
{
	L3MMMessage::MessageType MTI = (L3MMMessage::MessageType)(0xbf & source.MTI());
	DCOUT("parseL3MM MTI=" << MTI);

	L3MMMessage *retVal = L3MMFactory(MTI);
	if (retVal==NULL) return NULL;

	retVal->parse(source);
	DCOUT(*retVal);
	return retVal;
}



void L3MMMessage::text(ostream& os) const
{
	os << "MM " << (MessageType) MTI() << " ";
}


void L3LocationUpdatingRequest::parseBody( const  L3Frame &src, size_t &rp )
{
		// skip updating type
		rp += 4;
		// skip ciphering ket sequence number
		rp += 4;
		// skip LAI
		rp += 5*8;
		// skip classmark
		rp += 1*8;
		// Now parse MobileIdentity.
		mMobileIdentity.parseLV(src, rp);
}


void L3LocationUpdatingRequest::text(ostream& os) const
{
	L3MMMessage::text(os);
	os<<"MobileIdentity=("<<mMobileIdentity<<")";
}


size_t L3LocationUpdatingRequest::bodyLength() const
{
	size_t len = 0;
	len += 1;	// updating type and ciphering key seq num
	len += 5;	// LAI
	len += 1;	// classmark
	len +=  mMobileIdentity.lengthLV(); 
	return len;
}



void L3LocationUpdatingAccept::writeBody( L3Frame &dest, size_t &wp ) const
{
	mLAI.writeV(dest, wp);
}

void L3LocationUpdatingAccept::text(ostream& os) const
{
	L3MMMessage::text(os);
	os<<"LAI=("<<mLAI<<")";
}


void L3LocationUpdatingReject::writeBody( L3Frame &dest, size_t &wp ) const 
{
	mRejectCause.writeV(dest, wp);
}

void L3LocationUpdatingReject::text(ostream& os) const
{
	L3MMMessage::text(os);
	os <<"cause="<<mRejectCause;
}



void L3IMSIDetachIndication::parseBody(const L3Frame& src, size_t &rp)
{
	// skip classmark
	rp += 8;
	mMobileIdentity.parseLV(src, rp);
}

void L3IMSIDetachIndication::text(ostream& os) const
{
	L3MMMessage::text(os);
	os << "mobileID=(" << mMobileIdentity << ")";
}



void L3CMServiceReject::writeBody(L3Frame& dest, size_t &wp) const
{
	mCause.writeV(dest,wp);
}

void L3CMServiceReject::text(ostream& os) const
{
	L3MMMessage::text(os);
	os << "cause=" << mCause;
}



void L3CMServiceRequest::parseBody( const L3Frame &src, size_t &rp )
{
	rp += 4;			// skip ciphering key seq number
	mServiceType.parseV(src,rp);
	skipLV(src,rp);		// skip classmark
	mMobileIdentity.parseLV(src, rp);
	// ignore priority
}

void L3CMServiceRequest::text(ostream& os) const
{
	L3MMMessage::text(os);
	os << "serviceType=" << mServiceType;
	os << " mobileIdentity=("<<mMobileIdentity<<")";
}




void L3CMReestablishmentRequest::parseBody(const L3Frame& src, size_t &rp)
{
	rp += 8;			// skip ciphering
	skipLV(src,rp);		// skip classmark
	mMobileID.parseLV(src,rp);
	mHaveLAI = mLAI.parseTLV(0x13,src,rp);
}

void L3CMReestablishmentRequest::text(ostream& os) const
{
	L3MMMessage::text(os);
	os << "mobileID=(" << mMobileID << ")";
	if (mHaveLAI) os << " LAI=(" << mLAI << ")";
}


void L3MMStatus::parseBody( const L3Frame &src, size_t &rp)
{
	mRejectCause.parseV(src, rp);
}

void L3MMStatus::text(ostream& os) const 
{
	L3MMMessage::text(os);
	os << " RejectCause= <"<<mRejectCause<<">";
}


size_t L3MMInformation::bodyLength() const
{
	size_t len=0;
	if (mShortName.lengthV()>1) len += mShortName.lengthTLV();
	return len;
}


void L3MMInformation::writeBody(L3Frame &dest, size_t &wp) const
{
	if (mShortName.lengthV()>1) mShortName.writeTLV(0x45,dest,wp);
}


void L3MMInformation::text(ostream& os) const
{
	os << "short name=(" << mShortName << ")";
}


void L3IdentityRequest::writeBody(L3Frame& dest, size_t &wp) const
{
	dest.writeField(wp,0,4);		// spare half octet
	dest.writeField(wp,mType,4);
}


void L3IdentityRequest::text(ostream& os) const
{
	os << "type=" << mType;
}



void L3IdentityResponse::parseBody(const L3Frame& src, size_t& rp)
{
	mMobileID.parseLV(src,rp);
}

void L3IdentityResponse::text(ostream& os) const
{
	os << "mobile id=" << mMobileID;
}






// vim: ts=4 sw=4
