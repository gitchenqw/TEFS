TARGET = $(BIN)/$(APP)
APP=gfs
BIN=bin
PACKAGE=gfs
OBJ_PATH = objs
PREFIX_BIN = /usr/local/gfs
LOG_PATH=/var/log/gfs
ETC_PATH=/etc
CFGFILE=gfs.cfg
SPEC=gfs.spec
VERSION=1.0.0
RPMROOT=/root/rpmbuild
CONF_MACRO=

CC = gcc
INCLUDES = -I./core -I./net -I./event -I./busi -I./sql -I/usr/include/mysql
LIBS = -lpthread -lmysqlclient -lzlog -L/usr/lib64/mysql/
CFLAGS =-Wall -g $(CONF_MACRO) -DCLD_VERSION=\"$(VERSION).$(REV)\"
LINKFLAGS =

SRCDIR = core net event busi sql . 

#C_SOURCES = $(wildcard *.c)
C_SRCDIR = $(SRCDIR)
C_SOURCES = $(foreach d,$(C_SRCDIR),$(wildcard $(d)/*.c) )
C_OBJS = $(patsubst %.c, $(OBJ_PATH)/%.o, $(C_SOURCES))

#CPP_SOURCES = $(wildcard *.cpp)
CPP_SRCDIR = $(SRCDIR)
CPP_SOURCES = $(foreach d,$(CPP_SRCDIR),$(wildcard $(d)/*.cpp) )
CPP_OBJS = $(patsubst %.cpp, $(OBJ_PATH)/%.o, $(CPP_SOURCES))

default:init compile

$(C_OBJS):$(OBJ_PATH)/%.o:%.c
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@ $(LIBS)

$(CPP_OBJS):$(OBJ_PATH)/%.o:%.cpp
	$(CC) $(CFLAGS) -c $(INCLUDES) $< -o $@ $(LIBS)

init:
	$(foreach d,$(SRCDIR), mkdir -p $(OBJ_PATH)/$(d);)
	mkdir -p $(BIN)

test:
	@echo "C_SOURCES: $(C_SOURCES)"
	@echo "C_OBJS: $(C_OBJS)"
	@echo "CPP_SOURCES: $(CPP_SOURCES)"
	@echo "CPP_OBJS: $(CPP_OBJS)"
	
compile:$(C_OBJS) $(CPP_OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $^ $(LINKFLAGS) $(LIBS)

clean:
	rm -rf $(OBJ_PATH)
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX_BIN)
	mkdir -m 0777 -p $(DESTDIR)$(LOG_PATH)
	mkdir -p $(DESTDIR)$(ETC_PATH)
	cp -rf $(TARGET) $(DESTDIR)$(PREFIX_BIN)
	cp -rf ./cfg/$(CFGFILE) $(DESTDIR)$(ETC_PATH)

uninstall:
	rm -rf $(DESTDIR)$(PREFIX_BIN)
	rm -rf $(DESTDIR)$(ETC_PATH)/$(CFGFILE)
	rm -rf $(DESTDIR)$(LOG_PATH)

rebuild: clean init compile


dist:
	mkdir -p $(PACKAGE)-$(VERSION)
	mkdir -p $(PACKAGE)-$(VERSION)/bin
	$(foreach d, $(filter-out .,$(SRCDIR)), mkdir -p $(PACKAGE)-$(VERSION)/$(d); cp -rf $(d)/*.h $(d)/*.c* $(PACKAGE)-$(VERSION)/$(d);)
	mkdir -p $(PACKAGE)-$(VERSION)/cfg
	cp ./cfg/*.cfg $(PACKAGE)-$(VERSION)/cfg
	cp *.c $(PACKAGE)-$(VERSION)
	+cp *.spec $(PACKAGE)-$(VERSION)
	cp makefile $(PACKAGE)-$(VERSION)
	tar -zcvf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)

distclean:clean
	rm -rf $(PACKAGE)-$(VERSION).tar.gz

rpmbuild:dist
	mkdir -p rpm
	cp -rf $(SPEC) $(RPMROOT)/SPECS
	cp -rf $(PACKAGE)-$(VERSION).tar.gz $(RPMROOT)/SOURCES
	rpmbuild -ba $(RPMROOT)/SPECS/$(SPEC)
	@RELEASE=$(shell cat $(SPEC) | grep "Release:[0-9]*\$$" | cut -f2 -d":");\
	find $(RPMROOT) -name "$(PACKAGE)-$(VERSION)-$$RELEASE*.rpm" -type f -print0 | xargs -0 -I {} cp -rf {} rpm/
	rm -rf $(PACKAGE)-$(VERSION).tar.gz
rpmclean:clean
	rm -rf rpm  
