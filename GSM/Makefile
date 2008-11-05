ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets


TESTS = 


.PHONY: tests
tests:
	$(MAKE) $(TESTS)


.PHONY: objs
objs:
	$(MAKE) $(GSMOBJS)

# most GSM code includes these at a minimum
GSMINCS = $(COMMONINCS) GSMCommon.h GSMTransfer.h GSM610Tables.h


# specific rules for specific targets, where needed

GSML1FEC.o: GSML1FEC.cpp GSML1FEC.h GSMSAPMux.h GSMConfig.h GSMTDMA.h $(GSMINCS) \
		$(COMMON)/Threads.h $(TRXMANAGER)/TRXManager.h
	$(COMPILE) -o $@ $<

GSML1FECNoControl.o: GSML1FEC.cpp GSML1FEC.h GSMSAPMux.h GSMConfig.h GSMTDMA.h $(GSMINCS) \
		$(COMMON)/Threads.h $(TRXMANAGER)/TRXManager.h
	$(COMPILE) -DNOCONTROL -o $@ $<

GSMTransferNoL3.o: GSMTransfer.cpp $(GSMINCS)
	$(COMPILE) -DNOL3 -o $@ $<

GSMConfigNoL3.o: GSMConfig.cpp $(GSMINCS)
	$(COMPILE) -DNOL3 -o $@ $<



# standalone GSM applications
FULLAPPOBJS = $(GSMOBJS) $(COMMONOBJS) $(SIPOBJS) $(TRXMANAGEROBJS) $(CONTROLOBJS)

BlackRock: BlackRock.cpp $(GSMINCS) $(FULLAPPOBJS)
	$(BUILD) -o $@ $< $(FULLAPPOBJS) $(SIPLIBS)

MOCTest: MOCTest.cpp $(GSMINCS) $(FULLAPPOBJS)
	$(BUILD) -o $@ $< $(FULLAPPOBJS) $(SIPLIBS)




# default rule
%.o: %.cpp %.h $(GSMINCS)
	$(COMPILE) -o $@ $<



clean:
	-$(RM) *.o $(TESTS)
