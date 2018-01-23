PROTOS_DIR = protos
NANOPB_DIR = third_party/nanopb-0.3.9
MINIZ_DIR = third_party/miniz-2.0.6

CFLAGS = -O2 -Wall -g \
		 -I$(NANOPB_DIR) \
		 -I$(PROTOS_DIR) \
		 -I$(MINIZ_DIR) \
		 -D_FILE_OFFSET_BITS=64 \
		 -DMINIZ_NO_STDIO \
		 -DMINIZ_NO_ARCHIVE_APIS \
		 -DMINIZ_NO_ARCHIVE_WRITING_APIS \
		 -DMINIZ_NO_ZLIB_COMPATIBLE_NAMES \
		 -DMINIZ_NO_TIME

LDFLAGS = -L./$(PROTOS_DIR) -losmprotos

SRCS = $(MINIZ_DIR)/miniz_tinfl.c \
	   $(MINIZ_DIR)/miniz_tdef.c \
	   $(MINIZ_DIR)/miniz.c \
       src/osm_error.c \
	   src/fileblock.c \
	   src/blob.c \
	   src/primitive_block.c \
	   src/main.c

OBJS = $(SRCS:.c=.o)

TARGET = osmnano

default: $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "    CC $<"

protobufs:
	$(MAKE) -C $(PROTOS_DIR)

$(OBJS): protobufs

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o$(TARGET) $(OBJS) $(LDFLAGS)
	@echo "TARGET $@"

clean:
	$(MAKE) -C $(PROTOS_DIR) clean
	rm -f $(OBJS) $(TARGET)

.PRECIOUS: $(PROTOS)
