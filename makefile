# makefile

# uncomment the version of the compiler that you are using
#clang++ is for Mac OS 
#C++ = clang++ -std=c++11
# g++ is for most Linux
C++ = g++ -std=c++11

all: dataserver client test_suite

clean: 
	rm *.o

test_suite: sync_test_pcb sync_test_s

semaphore: semaphore.hpp semaphore.cpp
	$(C++) -c -g semaphore.cpp

pcbuffer: pcbuffer.hpp pcbuffer.cpp
	$(C++) -c -g pcbuffer.cpp

sync_test_pcb: sync_test.cpp pcbuffer.o
	$(C++) -DPCBUFFER_TEST -o sync_test_pcb sync_test.cpp pcbuffer.o -lpthread

sync_test_s: sync_test.cpp semaphore.o 
	$(C++) -DSEMAPHORE_TEST -o sync_test_s sync_test.cpp semaphore.o -lpthread

reqchannel.o: reqchannel.hpp reqchannel.cpp
	$(C++) -c -g reqchannel.cpp

dataserver: dataserver.cpp reqchannel.o 
	$(C++) -o dataserver dataserver.cpp reqchannel.o -lpthread

simpleclient: simpleclient.cpp reqchannel.o
	$(C++) -o simpleclient simpleclient.cpp reqchannel.o
