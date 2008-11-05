ROOT = ..
include $(ROOT)/Makefile.include


APPS = OpenBTS900 transceiver
.PHONY: apps
apps:
	$(MAKE) $(APPS)


include $(ROOT)/Makefile.targets


OPENBTSOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o  \
		$(GSMOBJS) $(CONTROLOBJS) $(SIPOBJS)
OpenBTS900: OpenBTS900.cpp $(COMMONINCS) $(OPENBTSOBJS)
	$(BUILD) -o $@ $< $(OPENBTSOBJS) $(STDLIBS) $(SIPLIBS)

transceiver:
	$(MAKE) -C $(ROOT)/Transceiver runTransceiver
	- ln -s $(ROOT)/Transceiver/runTransceiver ./transceiver


.PHONY: clean
clean:
	- $(RM) *.o *.~ temp 
	- $(RM) $(APPS)

