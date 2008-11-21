ROOT = ..
include $(ROOT)/Makefile.include
include $(ROOT)/Makefile.targets


TESTS = VectorTest TimevalTest SocketsTest BitVectorTest InterthreadTest



.PHONY: tests
tests:
	$(MAKE) $(TESTS)

.PHONY: objs
objs:
	$(MAKE) $(COMMONOBJS)


VectorTest: VectorTest.cpp Vector.h
	$(BUILD) -o $@ $<


TimevalTest: TimevalTest.cpp Timeval.h Timeval.o
	$(BUILD) -o $@ $< Timeval.o


SocketsTest: SocketsTest.cpp Sockets.h Threads.h Timeval.h Sockets.o Timeval.o Threads.o
	$(BUILD) -o $@ $< Sockets.o Timeval.o Threads.o



BitVector.o: BitVector.cpp BitVector.h Vector.h
	$(COMPILE) -O3 -o $@ $<

BitVectorTest: BitVectorTest.cpp Vector.h BitVector.o
	$(BUILD) -o $@ $< BitVector.o


InterthreadTest: InterthreadTest.cpp Interthread.h Threads.h Threads.o Timeval.h Timeval.o LinkedLists.h LinkedLists.o
	$(BUILD) -o $@ $< LinkedLists.o Threads.o Timeval.o


# default rule
%.o: %.cpp %.h 
	$(COMPILE) -o $@ $<


clean:
	- $(RM) *.o $(TESTS)
	- $(RM) testSource testDestination
