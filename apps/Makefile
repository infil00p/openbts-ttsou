ROOT = ..
include $(ROOT)/Makefile.include


APPS = OpenBTS900 OpenBTS850 transceiver
.PHONY: apps
apps:
	$(MAKE) $(APPS)


include $(ROOT)/Makefile.targets


OPENBTSOBJS = $(COMMONOBJS) $(TRXMANAGER)/TRXManager.o  \
		$(GSMOBJS) $(CONTROLOBJS) $(SIPOBJS)
OpenBTS900: OpenBTS900.cpp $(COMMONINCS) $(OPENBTSOBJS)
	$(BUILD) -o $@ $< $(OPENBTSOBJS) $(STDLIBS) $(SIPLIBS)
OpenBTS850: OpenBTS850.cpp $(COMMONINCS) $(OPENBTSOBJS)
	$(BUILD) -o $@ $< $(OPENBTSOBJS) $(STDLIBS) $(SIPLIBS)

transceiver:
	$(MAKE) -C $(ROOT)/Transceiver transceiver
	$(RM) ./transceiver
	- ln -s $(ROOT)/Transceiver/transceiver ./transceiver


.PHONY: clean
clean:
	- $(RM) *.o *.~ temp 
	- $(RM) $(APPS)

