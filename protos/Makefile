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

PROTO_SRCS = $(PROTOS:.proto=.pb.c)
PROTO_OBJS = $(PROTO_SRCS:.c=.o)
NANOPB_OBJS = $(NANOPB_CORE:.c=.o)

TARGET = libosmprotos.a

$(TARGET): $(PROTO_OBJS) $(NANOPB_OBJS)
	@$(AR) rc $@ $(PROTO_OBJS) $(NANOPB_OBJS)
	@$(RANLIB) $@
	@echo "STATIC $@"

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "    CC $<"

$(PROTO_OBJS): $(PROTO_SRCS)

clean:
	rm -f $(PROTO_SRCS) $(PROTO_OBJS) $(NANOPB_OBJS) $(TARGET)

.PRECIOUS: *.pb.h
