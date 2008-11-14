export ROOT = .
include Makefile.include


# when we're done, this builds to-level binaries
.PHONY: all
all: tests apps

include Makefile.targets


,PHONY: apps
apps:
	$(MAKE) -C apps apps

.PHONY: objs
objs:
	$(MAKE) -C $(COMMON) objs
	$(MAKE) -C $(CONTROL) objs
	$(MAKE) -C $(GSM) objs
	#$(MAKE) -C $(SMS) objs
	$(MAKE) -C $(TRXMANAGER) objs
	$(MAKE) -C $(TRANSCEIVER) objs

.PHONY: tests
tests:
	$(MAKE) -C $(COMMON) tests
	$(MAKE) -C $(CONTROL) tests
	$(MAKE) -C $(GSM) tests
	#$(MAKE) -C $(SMS) tests
	$(MAKE) -C $(TRXMANAGER) tests
	$(MAKE) -C $(TRANSCEIVER) tests
	$(MAKE) -C tests tests

.PHONY: checkin
checkin:
	$(MAKE) clean
	$(MAKE) objs
	$(MAKE) tests
	$(MAKE) apps
	svn commit


.PHONY: dox
dox: doxconfig
	-$(RM) dox_html/*
	doxygen doxconfig


.PHONY: linecount
linecount:
	grep ";" */*.cpp */*.h | wc -l

.PHONY: complexity
complexity:
	gzip -c */*.cpp */*.h | wc -c



.PHONY: clean
clean:
	- $(RM) *.o
	- $(MAKE) -C $(COMMON) clean
	- $(MAKE) -C $(GSM) clean
	- $(MAKE) -C $(CONTROL) clean
	- $(MAKE) -C $(TRXMANAGER) clean
	- $(MAKE) -C $(TRANSCEIVER) clean
	- $(MAKE) -C $(SIP) clean
	- $(MAKE) -C $(SMS) clean
	- $(MAKE) -C $(APPS) clean
	


