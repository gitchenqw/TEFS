# Copyright (c) 1999 Apple Computer, Inc.  All rights reserved.
#  
PREFIX_BIN = /usr/bin
PREFIX_ETC = /etc/gvfs/dataserver
#MYSQLPATH = $(DESTDIR)$(PREFIX_ETC)/mysql
NAME = dataserver
CXX = g++
CC = $(CCOMP)
LINK = $(LINKER)
CCFLAGS += $(COMPILER_FLAGS) $(INCLUDE_FLAG) -g -Wall -Wno-write-strings -D_EPOLL -std=c++0x -D__STDC_FORMAT_MACROS
LIBS = $(CORE_LINK_LIBS)  -lCommonUtilitiesLib -L../CommonUtilitiesLib/ \
-L../diskmanager/src/ -lds \
  -lpthread
#-lmysqlclient -L../mysql/lib/
#OPTIMIZATION
CCFLAGS += 

# EACH DIRECTORY WITH HEADERS MUST BE APPENDED IN THIS MANNER TO THE CCFLAGS

CCFLAGS += -I.
CCFLAGS += -I..
CCFLAGS += -I../CommonUtilitiesLib
CCFLAGS += -I../mysql/include
CCFLAGS += -I../APICommonCode
CCFLAGS += -I../PrefsSourceLib
CCFLAGS += -I../Pref
CCFLAGS += -I../message
CCFLAGS += -I../diskmanager/src/
CCFLAGS += -I../diskmanager/
CCFLAGS += -I../diskmanager/common
# EACH DIRECTORY WITH A STATIC LIBRARY MUST BE APPENDED IN THIS MANNER TO THE LINKOPTS

CXXFLAGS = $(CCFLAGS)

CFILES = 

CPPFILES = ../message/Message.cpp ../message/ClientMessage.cpp ../message/NsMessage.cpp\
           ../message/DsMessage.cpp ../SafeStdLib/InternalStdLib.cpp \
           DSTCPListener.cpp DSTCPServerSocket.cpp DSServer.cpp main.cpp \
           DSMessageProcess.cpp ../PrefsSourceLib/FilePrefsSource.cpp \
           DSServerPref.cpp  ../APICommonCode/QTSSRollingLog.cpp DSErrorLog.cpp bserveripPref.cpp\
           DSTCPSession.cpp  DSClientPutTask.cpp DSClientGetTask.cpp DSPushTask.cpp DSPutTask.cpp \
           NameServersession.cpp
           
TESTCPPFILES =  ../SafeStdLib/InternalStdLib.cpp DSServerTest.cpp ../message/Message.cpp ../message/ClientMessage.cpp ../message/NsMessage.cpp
# CCFLAGS += $(foreach dir,$(HDRS),-I$(dir))

all:  dataserver



dataserver: $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
	$(CXX) -o $@ $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(COMPILER_FLAGS) $(LIBS) -g
    
test:$(TESTCPPFILES:.cpp=.o)
	$(CXX) -o $@ $(TESTCPPFILES:.cpp=.o) $(COMPILER_FLAGS) $(LIBS) -g
    
.SUFFIXES: .cpp .c .o

.cpp.o:
	$(CXX) -o $@ -c $*.cpp $(LIBS) $(CXXFLAGS)  

.c.o:
	$(CC) -c $(DEFINES) $(CCFLAGS) $*.c

install:
	mkdir -p $(DESTDIR)$(PREFIX_BIN)
	mkdir -p $(DESTDIR)$(PREFIX_ETC)
#	mkdir -p $(MYSQLPATH)
	cp  dataserver $(DESTDIR)$(PREFIX_BIN)
	if [ -f $(DESTDIR)$(PREFIX_ETC)/dataserver.conf ]; then \
 	    echo "if cover the configure files?(yes/no)";      \
	    read a;                                             \
	    while [ $$a != "yes" ] && [ $$a != "no" ];            \
	    do                                                 \
	    echo 'please enter "yes" or "no" ';                 \
	    read a;                                             \
	    done;                                               \
	    if [ $$a = "yes" ]; then                             \
                cp  ../Pref/dataserver.conf $(DESTDIR)$(PREFIX_ETC); \
	    fi                                                 \
	else                                               \
	    cp ../Pref/dataserver.conf $(DESTDIR)$(PREFIX_ETC); \
	fi
	cp ./Readme.txt   $(DESTDIR)$(PREFIX_ETC);
#	cp -rf ../mysql/* $(MYSQLPATH)
    
clean:
	rm -f $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(TESTCPPFILES:.cpp=.o)
