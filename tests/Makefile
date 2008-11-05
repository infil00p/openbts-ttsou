ROOT = ..
include $(ROOT)/Makefile.include


TESTS = TRXSimulator LoopbackTest BeaconTest AGCHTest CallTest RegistrationTest LAPDmTest CallTest2
.PHONY: tests
tests:
	$(MAKE) $(TESTS)


include $(ROOT)/Makefile.targets


LOOPBACKTESTOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o \
		$(GSM)/GSML1FECNoControl.o $(GSM)/GSMCommon.o $(GSM)/GSMTransferNoL3.o \
		$(GSM)/GSMConfigNoL3.o $(GSM)/GSML3RRElements.o $(GSM)/GSML3CommonElements.o \
		$(GSM)/GSMTDMA.o $(GSM)/GSMSAPMux.o $(GSM)/GSM610Tables.o
LoopbackTest: LoopbackTest.cpp $(COMMONINCS) $(LOOPBACKTESTOBJS)
	$(BUILD) -DNOCONTROL -DNOL3 -o $@ $< $(LOOPBACKTESTOBJS) $(STDLIBS)

BEACONTESTOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o \
		$(GSM)/GSML1FECNoControl.o $(GSM)/GSMCommon.o $(GSM)/GSMTransfer.o \
		$(GSM)/GSMConfig.o $(GSM)/GSMTDMA.o $(GSM)/GSM610Tables.o \
		$(GSML3OBJS) 
BeaconTest: BeaconTest.cpp $(COMMONINCS) $(BEACONTESTOBJS)
	$(BUILD) -DNOCONTROL -o $@ $< $(BEACONTESTOBJS) $(STDLIBS)

AGCHTESTOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o  \
		$(GSM)/GSML1FEC.o $(GSM)/GSMCommon.o $(GSM)/GSMTransfer.o \
		$(GSM)/GSMConfig.o $(GSM)/GSMTDMA.o $(GSM)/GSM610Tables.o \
		$(GSM)/GSMLogicalChannel.o $(GSM)/GSML2LAPDm.o $(GSM)/GSMSAPMux.o \
		$(CONTROL)/RadioResourcePagerTest.o \
		$(GSML3OBJS) 
AGCHTest: AGCHTest.cpp $(COMMONINCS) $(AGCHTESTOBJS)
	$(BUILD) -o $@ $< $(AGCHTESTOBJS) $(STDLIBS)

REGISTRATIONTESTOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o  \
		$(GSMOBJS) $(CONTROLOBJS) $(SIPOBJS)
RegistrationTest: RegistrationTest.cpp $(COMMONINCS) $(REGISTRATIONTESTOBJS)
	$(BUILD) -o $@ $< $(REGISTRATIONTESTOBJS) $(STDLIBS) $(SIPLIBS)

CALLTESTOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o  \
		$(GSMOBJS) $(CONTROLOBJS) $(SIPOBJS)
CallTest: CallTest.cpp $(COMMONINCS) $(CALLTESTOBJS)
	$(BUILD) -o $@ $< $(CALLTESTOBJS) $(STDLIBS) $(SIPLIBS)
CallTest2: CallTest2.cpp $(COMMONINCS) $(CALLTESTOBJS)
	$(BUILD) -o $@ $< $(CALLTESTOBJS) $(STDLIBS) $(SIPLIBS)


TRXSIMOBJS = $(COMMONOBJS) $(GSM)/GSMCommon.o
TRXSimulator: TRXSimulator.cpp $(COMMONINCS) $(TRXSIMOBJS)
	$(BUILD) -o $@ $< $(TRXSIMOBJS) $(STDLIBS)


LAPDMTESTOBJS = $(COMMONOBJS) \
		$(GSM)/GSMCommon.o $(GSM)/GSML2LAPDm.o $(GSM)/GSMTransfer.o $(GSM)/GSMSAPMux.o
LAPDmTest: LAPDmTest.cpp $(COMMONINCS) $(LAPDMTESTOBJS)
	$(BUILD) -o $@ $< $(LAPDMTESTOBJS)


.PHONY: clean
clean:
	- $(RM) *.o *.~ temp 
	- $(RM) $(TESTS)

