ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets
	 

GSML3OBJS_SMS = $(GSMCORE)/GSML3Processors.o $(GSMCORE)/GSML3MessageSMS.o $(GSMCORE)/GSML3RRMessages.o $(GSMCORE)/GSML3MMMessages.o $(GSMCORE)/GSML3CCMessages.o $(GSMCORE)/GSML3CommonElements.o $(GSMCORE)/GSML3RRElements.o $(GSMCORE)/GSML3MMElements.o $(GSMCORE)/GSML3CCElements.o

GSMCOREOBJS_SMS = $(GSMCORE)/GSMCommon.o $(GSMCORE)/GSMTransfer.o $(GSMCORE)/GSMConfig.o $(GSMCORE)/GSML1FECs.o $(GSMCORE)/GSML2LAPDms.o $(GSML3OBJS_SMS) $(GSMCORE)/GSMLogicalChannel.o $(TRXOBJS) $(COMMSIG)/CommSig.o $(COMMON)/Common.o $(GSMCORE)/GSMChanMgr.o $(GSMCORE)/GSMSAPMux.o


SMS_OBJS= 	CMMessage.o RLMessage.o TLMessage.o 		\
		CMProcessor.o RLProcessor.o TLProcessor.o	\
		SMSTransfer.o 					\
		$(GSMCOREOBJS_SMS)				\
		$(CONTROLLOCALOBJS) 				\
		$(SIPOBJS)

CMTest: CMTest.cpp  $(SMS_OBJS)  
	$(BUILD)  -DLOCAL -o $@ $< $(SMS_OBJS)  -lgsm -lpthread -losipparser2 -lortp -losip2







# default rule
%.o: %.cpp %.h $(COMMON)/Common.h
	$(COMPILE) -o $@ $<


.PHONY: linecount
linecount:
	grep ";" *.cpp *.h | wc -l


# phony utility target

.PHONY: clean
clean:
	- $(RM) *.o *~
	- $(RM) $(TESTS)
	- $(RM) *.swp *.out
	- $(RM) *.au
	- $(RM) -rf *.dSYM
