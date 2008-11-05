ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets


TESTS = 
.PHONY: tests
tests:
	$(MAKE) $(TESTS)

.PHONY: objs
objs:
	$(MAKE) $(TRXMANAGEROBJS)


TRXManager.o: TRXManager.cpp TRXManager.h $(GSM)/GSMTDMA.h $(GSM)/GSML1FEC.h
	$(COMPILE) -o $@ $<


# default rule
%.o: %.cpp %.h
	$(COMPILE) -o $@ $<

.PHONY: clean
clean:
	- $(RM) *.o *.~ temp 
	- $(RM) $(TESTS)

