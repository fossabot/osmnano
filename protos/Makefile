NANOPB_DIR = ../third_party/nanopb-0.3.9
include $(NANOPB_DIR)/extra/nanopb.mk

CFLAGS = -O2 -Wall -g \
		 -I$(NANOPB_DIR) \
		 -DPB_FIELD_32BIT \
		 -DPB_BUFFER_ONLY

PROTOS = fileformat.proto \
		 osmformat.proto \
		 osmnano.proto

RANLIB = ranlib

SRCS = $(PROTOS:.proto=.pb.c)

OBJS = $(NANOPB_CORE:.c=.o) $(SRCS:.c=.o)

TARGET = libosmprotos.a

default: $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "    CC $<"

$(TARGET): $(OBJS)
	@$(AR) rc $@ $(OBJS)
	@$(RANLIB) $@
	@echo "STATIC $@"

clean:
	rm -f $(SRCS) $(OBJS) $(SRCS:.pb.c=.pb.h) $(TARGET)

.PRECIOUS: *.pb.h
