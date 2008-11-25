#
# Copyright 2008 Free Software Foundation, Inc.
#
# This software is distributed under the terms of the GNU Public License.
# See the COPYING file in the main directory for details.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

include $(top_srcdir)/Makefile.common

AM_CPPFLAGS = $(STD_DEFINES_AND_INCLUDES)

EXTRA_DIST = README.Control

noinst_LTLIBRARIES = libcontrol.la

noinst_PROGRAMS = PagerTest

libcontrol_la_SOURCES = \
	CallControl.cpp \
	ControlCommon.cpp \
	FACCHDispatch.cpp \
	MobilityManagement.cpp \
	RadioResource.cpp \
	SDCCHDispatch.cpp

PagerTest_SOURCES = PagerTest.cpp
PagerTest_LDADD = \
	libcontrol.la \
	$(COMMON_LA) \
	$(GSM_LA) \
	$(SIP_LA) \
	$(TRX_LA) \
	$(OSIP_LIBS) \
	$(ORTP_LIBS)

PagerTest_LDFLAGS = -lpthread

noinst_HEADERS = \
	ControlCommon.h
