PROTOS_DIR = protos
NANOPB_DIR = third_party/nanopb-0.3.9
MINIZ_DIR = third_party/miniz-2.0.6

CFLAGS = -O2 -Wall -g -std=c99 \
		 -Wno-strict-aliasing \
		 -I$(NANOPB_DIR) \
		 -I$(PROTOS_DIR) \
		 -I$(MINIZ_DIR) \
		 -D_GNU_SOURCE \
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
	   src/task_server.c \
	   src/task_worker.c \
       src/osm_error.c \
	   src/fileblock.c \
	   src/blob.c \
	   src/primitive_block.c \
	   src/main.c

OBJS = $(SRCS:.c=.o)

TARGET = osmnano

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) -o$(TARGET) $(OBJS) $(LDFLAGS)
	@echo "TARGET $@"

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo "    CC $<"

$(PROTOS_DIR)/libosmprotos.a:
	$(MAKE) -C $(PROTOS_DIR)

$(OBJS): $(PROTOS_DIR)/libosmprotos.a

clean:
	$(MAKE) -C $(PROTOS_DIR) clean
	rm -f $(OBJS) $(TARGET)

.PRECIOUS: $(PROTOS)
