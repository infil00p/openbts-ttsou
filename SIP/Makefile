ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets


.PHONY: tests
tests:
	$(MAKE) $(TESTS)

.PHONY: objs
objs:
	$(MAKE) $(SIPOBJS)



# test targets

#=========================================================================
#       Controller Test: Migth not put any standalone tests for these 
#       dispatch handlers.
#=========================================================================

SIPTEST_OBJS =  $(COMMON)/Common.o	\

SIPTEST_DEP =	$(SIPTEST_OBJS)

SIPTest:SIPInterface.cpp $(SIPTEST_DEP)
	$(BUILD) -DUNIT_TEST -o $@ $< $(SIPTEST_OBJS) \
	-lpthread /usr/local/lib/libosipparser2.a /usr/local/lib/libosip2.a  


TESTSIPEngine.o:SIPEngine.cpp SIPEngine.h
	$(COMPILE) -o $@ -DUNIT_TEST $<
	

TEST_OBJS =$(GSMOBJS)$(COMMONOBJS) $(VOCODEROBJS) $(CONTROLOBJS) \
	SIPInterface.o SIPUtility.o SIPMessage.o 


MOC_OBJS=$(COMMON)/Common.o $(COMMON)/Sockets.o TESTSIPEngine.o \
	SIPInterface.o SIPMessage.o SIPUtility.o


# Standalone test for sip messages.
MSG_OBJS=SIPMessage.o $(COMMON)/Common.o

MSGTest:MSGTest.cpp $(MSG_OBS) 
	$(BUILD) -DUNIT_TEST -o $@ $< $(MSG_OBJS)  \
	/usr/local/lib/libortp.a -lpthread  \
	/usr/local/lib/libosipparser2.a /usr/local/lib/libosip2.a  

MOCTest: MOCTest.cpp $(MOC_OBJS)
	$(BUILD) -DUNIT_TEST -o $@ $< $(MOC_OBJS)  \
	/usr/local/lib/libortp.a -lpthread -lgsm \
	/usr/local/lib/libosipparser2.a /usr/local/lib/libosip2.a


REGTest: REGTest.cpp $(TEST_OBJS)
	$(BUILD) -DUNIT_TEST -o $@ $< $(TEST_OBJS) \
	/usr/local/lib/libortp.a -lpthread \
	/usr/local/lib/libosipparser2.a /usr/local/lib/libosip2.a




#=========================================================================
#=========================================================================



# default rule
%.o: %.cpp %.h SIPEngine.h SIPInterface.h SIPMessage.h SIPUtility.h
	$(COMPILE) -o $@ $<


.PHONY: clean
clean:
	- $(RM) *.o
	- $(RM) *~ *.out 
	- $(RM) $(TESTS)
	- $(RM) -rf *.dSYM

