ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets

TESTS = PagerTest

.PHONY: tests
tests:
	$(MAKE) $(TESTS)

.PHONY: objs
objs:
	$(MAKE) $(CONTROLOBJS)



# test targets

#===============================================================================
#	PagerTest: ... something
#
#===============================================================================

# A special version of RadioResource.o that does not invoke CallControl.o.
# This is just for use in the PagerTest application.
RadioResourcePagerTest.o: RadioResource.cpp ControlCommon.h
	$(COMPILE) -DPAGERTEST -o $@ $<

PAGERTESTOBJS = $(GSMOBJS) $(COMMONOBJS) $(SIPOBJS)	\
		$(TRXMANAGER)/TRXManager.o		\
		ControlCommon.o				\
		RadioResourcePagerTest.o

PAGERTEST_DEP =	ControlCommon.h			\
		$(PAGERTESTOBJS)

PagerTest: PagerTest.cpp $(PAGERTEST_DEP) 
	$(BUILD) -DPAGERTEST -o $@ $< $(PAGERTESTOBJS) $(SIPLIBS)

#=========================================================================
#       Controller Test: Might not put any standalone tests for these 
#       dispatch handlers.
#=========================================================================

CONTROLLER_DEP = DispatchController.cpp			\
		 DispatchController.h			\
		 $(GSM)/GSMLogicalChannel.h                    \
                 $(GSM)/GSML3CommonElements.h       \
                 $(GSM)/GSMCommon.h                 \
                 $(COMMSIG)/CommSig.h			\
               	 $(GLOBALS)/SQLInterface.h		 

# if the dispat handers will deal with a database, then they need to 
# have SQLInterface.h not error out, so we need to include these 
# paths for compilation.

SQL_LIBS =      -I/usr/include/mysql    \
                -L/usr/lib/mysql        \

DispatchController.o:$(CONTROLLER_DEP) 
	$(COMPILE) -o $@ $< $(SQL_LIBS)



#=========================================================================
#       Controller Test: Might not put any standalone tests for these 
#       dispatch handlers.
#=========================================================================


MOCTEST_DEP = 	 moc.cpp				\
		 SDCCHController.h			\
		 $(GSM)/GSMLogicalChannel.h		\
                 $(GSM)/GSML3CommonElements.h       \
                 $(GSM)/GSMCommon.h                 \
                 $(COMMSIG)/CommSig.h			\
               	 $(GLOBALS)/SQLInterface.h		\
		 $(MOCTEST_OBJS)			

MOCTEST_OBJS =	$(GSMOBJS_SLOW)			\
		$(VOCODEROBJS)			\
		$(GLOBALS)/SQLInterface.o	\
		$(SIP)/SIPInterface.o	\
		$(SIP)/SIPMessage.o		\
		SDCCHController.o		\
	

SQL_LIBS =      -I/usr/include/mysql    \
                -L/usr/lib/mysql        \

MOCTest:$(MOCTEST_DEP)  $(GSM610_CODEC)/lib/libgsm.a 
	$(BUILD) -o $@ $< $(SQL_LIBS) $(MOCTEST_OBJS) -lmysqlclient -lgsm $(SIPLIBS)

moc.o:$(MOCTEST_DEP) 
	$(COMPILE) -o $@ $< $(SQL_LIBS)



TLTEST_DEP=	TCHLoopback.cpp		\
		$(MOCTEST_OBJS)

TLTest:$(TLTEST_DEP)  
	$(BUILD) -o $@ $< $(SQL_LIBS) $(MOCTEST_OBJS) -lmysqlclient -lgsm 
	
#==============================================================================
#==============================================================================


CONTROL_INCS =  ControlCommon.h 		\
		$(SIP)/SIPUtility.h 		\
		$(SIP)/SIPEngine.h 		\
		$(GSM)/GSMLogicalChannel.h 	\
		$(GSM)/GSML3Message.h 		
	
SMS_INCS = 	$(SMS)/CMMessage.h $(SMS)/RLMessage.h $(SMS)/TLMessage.h 	\
		$(SMS)/CMProcessor.h $(SMS)/RLProcessor.h $(SMS)/TLProcessor.h 	\
		$(SMS)/SMSTransfer.h




ControlCommon.o: ControlCommon.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

ControlCommonLocal.o: ControlCommon.cpp $(CONTROL_INCS)
	$(COMPILE) -DLOCAL -o $@ $<

CallControl.o: CallControl.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

CallControlLocal.o: CallControl.cpp $(CONTROL_INCS)
	$(COMPILE) -DLOCAL -o $@ $<

CallControlSMS.o: CallControl.cpp $(CONTROL_INCS) $(SMS_INCS)
	$(COMPILE) -DSMS -o $@ $<

RadioResource.o: RadioResource.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

MobilityManagement.o: MobilityManagement.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

MobilityManagementLocal.o: MobilityManagement.cpp $(CONTROL_INCS)
	$(COMPILE) -DLOCAL -o $@ $<

MobilityManagementSMS.o: MobilityManagement.cpp $(CONTROL_INCS) $(SMS_INCS)
	$(COMPILE) -DSMS -o $@ $<

SDCCHDispatch.o: SDCCHDispatch.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

FACCHDispatch.o: FACCHDispatch.cpp $(CONTROL_INCS)
	$(COMPILE) -o $@ $<


# default rule
%.o: %.cpp %.h $(CONTROL_INCS)
	$(COMPILE) -o $@ $<

.PHONY: linecount
linecount:
	grep ";" *.cpp *.h | wc -l


.PHONY: clean
clean:
	- $(RM) *.o *.out
	- $(RM) *~
	- $(RM) $(TESTS)


