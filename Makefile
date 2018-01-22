include third_party/nanopb-0.3.9/extra/nanopb.mk

PROTOS_DIR = protos
MINIZ_DIR = third_party/miniz-2.0.6

LDFLAGS = 

CFLAGS = -O2 -Wall -g \
		 -I$(NANOPB_DIR) \
		 -I$(PROTOS_DIR) \
		 -I$(MINIZ_DIR) \
		 -DMINIZ_NO_STDIO \
		 -DMINIZ_NO_ARCHIVE_APIS \
		 -DMINIZ_NO_ARCHIVE_WRITING_APIS \
		 -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES \
		 -DMINIZ_NO_TIME \
		 -DPB_FIELD_32BIT \
		 -DPB_BUFFER_ONLY

SRCS = $(NANOPB_DIR)/pb_common.c \
	   $(NANOPB_DIR)/pb_decode.c \
	   $(MINIZ_DIR)/miniz_tinfl.c \
	   $(MINIZ_DIR)/miniz_tdef.c \
	   $(MINIZ_DIR)/miniz.c \
	   src/fileformat.c \
	   src/fileblock.c \
	   src/osmformat.c \
	   src/intlist.c \
	   src/ptrlist.c \
	   src/main.c

PROTOS = $(PROTOS_DIR)/fileformat.pb.c \
		 $(PROTOS_DIR)/osmformat.pb.c

OBJS = $(SRCS:.c=.o) $(PROTOS:.c=.o)

TARGET = osmnano


%.pb.c: %.proto
	@$(PROTOC) $(PROTOC_OPTS) --nanopb_out=$(PWD) $<
	@echo "PROTOC $<"

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "    CC $<"

$(TARGET): generator $(PROTOS) $(OBJS)
	@$(CC) $(LDFLAGS) -o$(TARGET) $(OBJS)
	@echo "TARGET $@"

$(OBJS): $(PROTOS)

generator:
	$(MAKE) -C $(NANOPB_DIR)/generator/proto

clean:
	rm -f $(PROTOS) $(OBJS) $(TARGET) $(PROTOS_DIR)/*.pb.h 

.PRECIOUS: $(PROTOS)
