include ./Rules.mak

CORE_OBJS := $(CORE_SRCS:.c=.o)
EVENT_OBJS := $(EVENT_SRCS:.c=.o)
OS_OBJS := $(OS_SRCS:.c=.o)
HTTP_OBJS := $(HTTP_SRCS:.c=.o)
PRIVATE_OBJS := $(PRIVATE_SRCS:.c=.o)
COS_OBJS := $(COS_SRCS:.c=.o)

%.o: %.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) \
	-o $@ \
	$<

all: $(CORE_OBJS) $(EVENT_OBJS) $(OS_OBJS) $(HTTP_OBJS) $(PRIVATE_OBJS) $(COS_OBJS) gvfs_proxy

.PHONY: all clean

$(CORE_OBJS): %.o : %.c

$(EVENT_OBJS): %.o : %.c

$(OS_OBJS): %.o : %.c

$(HTTP_OBJS): %.o : %.c

$(PRIVATE_OBJS): %.o : %.c

$(COS_OBJS): %.o : %.c

gvfs_proxy: $(CORE_OBJS) $(EVENT_OBJS) $(OS_OBJS) $(HTTP_OBJS) $(PRIVATE_OBJS) $(COS_OBJS)
	$(CC) $(CFLAGS) $(ALL_INCS) \
	-o $@ \
	$(CORE_OBJS) \
	$(EVENT_OBJS) \
	$(OS_OBJS) \
	$(HTTP_OBJS) \
	$(PRIVATE_OBJS) \
	$(COS_OBJS) \
	-lpthread -lzlog -lcrypto -lz
	
clean:
	rm -rf \
	$(CORE_OBJS) \
	$(EVENT_OBJS) \
	$(OS_OBJS) \
	$(HTTP_OBJS) \
	$(PRIVATE_OBJS) \
	$(COS_OBJS) \
	gvfs_proxy