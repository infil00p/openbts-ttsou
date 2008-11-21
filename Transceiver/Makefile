ROOT = ..
include $(ROOT)/Makefile.include

all: transceiver

include $(ROOT)/Makefile.targets

#=================================================================
#                         Build Test Targets
#=================================================================

TESTS = USRPping  		\
	transceiver          


GSMOBJS = $(GSM)/GSMCommon.o

TESTRADIOOBJS = $(GSMOBJS) $(CONTROLOBJS)



.PHONY: tests
tests:
	$(MAKE) $(TESTS)

.PHONY: objs
objs:
	$(MAKE) $(TRANSCEIVEROBJS)

prepbits: prepbits.c
	gcc -o prepbits prepbits.c


BUILDOPTS = -O3 # -DSWLOOPBACK 
USRPLIBS = -lusrp


ifeq ($(shell uname -p),powerpc)
BUILDOPTS += -DWORDS_BIGENDIAN
endif




USRPping: USRPping.cpp USRPDevice.o $(GSMOBJS) $(COMMONOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< USRPDevice.o $(GSMOBJS) $(COMMONOBJS) $(USRPLIBS)

sweepGenerator: sweepGenerator.cpp USRPDevice.o $(GSMOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< USRPDevice.o $(GSMOBJS) $(USRPLIBS)

USRPTest: USRPTest.cpp USRPDevice.o $(GSMOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< USRPDevice.o $(GSMOBJS) $(USRPLIBS)

# sigProcLibTest
sigProcLibTest: sigProcLibTest.cpp sigProcLib.o $(GSMOBJS)
	$(BUILD) -I .. -DUNIT_TEST -o $@ $< sigProcLib.o $(GSMOBJS)

# testRadio
testRadio: testRadio.cpp $(TRANSCEIVEROBJS) $(TESTRADIOOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< $(TRANSCEIVEROBJS) $(TESTRADIOOBJS $(USRPLIBS))

# transceiver
transceiver: runTransceiver.cpp $(TRANSCEIVEROBJS) $(GSMOBJS) $(COMMONOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< $(TRANSCEIVEROBJS) $(GSMOBJS) $(COMMONOBJS) $(USRPLIBS)

TRANSCEIVERLOGGINGOBJS = $(TRANSCEIVER)/USRPDevice.o $(TRANSCEIVER)/sigProcLib.o $(TRANSCEIVER)/radioInterface.o TransceiverLogging.o
transceiverLogging: runTransceiver.cpp $(TRANSCEIVERLOGGINGOBJS) $(GSMOBJS) $(COMMONOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< $(TRANSCEIVERLOGGINGOBJS) $(GSMOBJS) $(COMMONOBJS) $(USRPLIBS)
TRANSCEIVERLOGGINGOBJS = $(TRANSCEIVER)/USRPDevice.o $(TRANSCEIVER)/sigProcLib.o $(TRANSCEIVER)/radioInterface.o TransceiverLogging1.o
transceiverLogging1: runTransceiver.cpp $(TRANSCEIVERLOGGINGOBJS) $(GSMOBJS) $(COMMONOBJS)
	$(BUILD) -I .. $(BUILDOPTS) -o $@ $< $(TRANSCEIVERLOGGINGOBJS) $(GSMOBJS) $(COMMONOBJS) $(USRPLIBS)



USRPDevice.o: USRPDevice.cpp USRPDevice.h $(COMMON)/Threads.h
	$(COMPILE) $(BUILDOPTS) -o $@ $<

USRPDeviceLoopback.o: USRPDevice.cpp USRPDevice.h $(COMMON)/Threads.h
	$(COMPILE) -DSWLOOPBACK $(BUILDOPTS) -o $@ $<


Transceiver.o: Transceiver.cpp Transceiver.h radioInterface.h $(COMMONINCS) $(GSM)/GSMCommon.h
	$(COMPILE) $(BUILDOPTS) -o $@ $<

TransceiverLogging.o: Transceiver.cpp Transceiver.h radioInterface.h $(COMMONINCS) $(GSM)/GSMCommon.h
	$(COMPILE) $(BUILDOPTS) -DTRANSMIT_LOGGING=0 -o $@ $<

TransceiverLogging1.o: Transceiver.cpp Transceiver.h radioInterface.h $(COMMONINCS) $(GSM)/GSMCommon.h
	$(COMPILE) $(BUILDOPTS) -DTRANSMIT_LOGGING=1 -o $@ $<


# default rule
%.o: %.cpp %.h
	$(COMPILE) $(BUILDOPTS) -o $@ $<

.PHONY: clean
clean:
	- $(RM) *.o
	- $(RM) $(TESTS)

