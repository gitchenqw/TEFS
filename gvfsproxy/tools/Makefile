include ./Rules.mak

OBJS := $(SRCS:.c=.o)
ENC_OBJS := $(ENC_SRC:.c=.o)
DEC_OBJ := $(DEC_SRC:.c=.o)

%.o: %.c
	$(CC) -c $(CFLAGS) $(ALL_INCS) \
	-o $@ \
	$<

all: $(OBJS) $(ENC_OBJS) $(DEC_OBJ) encrypt decrypt

.PHONY: all clean

$(OBJS): %.o : %.c

$(ENC_OBJS): %.o : %.c

$(DEC_OBJ): %.o : %.c

encrypt: $(OBJS)
	$(CC) $(CFLAGS) $(ALL_INCS) -o $@ $(ENC_OBJS) $(OBJS)  -lcrypto -lz

decrypt: $(OBJS)
	$(CC) $(CFLAGS) $(ALL_INCS) -o $@ $(DEC_OBJ) $(OBJS)  -lcrypto -lz
	
clean:
	rm -rf $(OBJS) encrypt decrypt