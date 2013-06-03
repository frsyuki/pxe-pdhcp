
OUTPUT = pxe-pdhcp

CFLAGS += -Wall -g -O4

SRCS = pdhcp.c pxe-pdhcp.c

OBJS = $(subst .c,.o,$(SRCS))

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(OUTPUT)


.PHONY: source distclean
source:
distclean: clean
