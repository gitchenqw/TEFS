#common Makefile

CPPFILES= atomic.cpp tbsys.cpp lock.cpp internal.cpp file_queue.cpp parameter.cpp directory_op.cpp func.cpp

OBJS= $(CPPFILES:.cpp=.o)

CXX= g++ 

SLIB=libcom.a
DLIB=libcom.so

CCFLAGS=  -g  -Wall  -std=c++0x

CCFLAGS += -I.
CCFLAGS += -I..

LIB = -lpthread 

all:$(OBJS)
	$(CXX) -o  $(DLIB) -shared  -Wl -fPIC $(OBJS) $(LIB)
	ar rc $(SLIB)  $(OBJS) 
.cpp.o:
	$(CXX) -c -o $*.o $(CCFLAGS) $(INCLUDEDIR) $*.cpp -fPIC
test:
#$(CXX) -o  $(TESTOUTPUT) $(CCFLAGS) test_blockfile_manager.cpp $(SLIB)
    
clean:
	rm -rf *.o $(DLIB) $(SLIB)
    



