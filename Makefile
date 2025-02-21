MAKEFLAGS += -rR
.SUFFIXES:

O := .O
CC := gcc
CFLAGS := -g -O0 -std=gnu11

exe := vtop

libs := jansson
OBJS := main.o util.o log.o vtop.o client.o proto.o

## Computed Variables

DEPS := $(patsubst %.o, %.d, $(addprefix $O/, $(OBJS)))

lib_cflags := $(shell pkg-config --cflags $(libs))
libs_ldflags := $(shell pkg-config --libs $(libs))

.PHONY: all, clean, distclean
all: $(exe)

$(exe): $(addprefix $O/, $(OBJS))
	$(CC) $(LDFLAGS) -o $@ $^ $(libs_ldflags)

$O/%.o: %.c
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(lib_cflags) -MMD -c $< -o $@

clean:
	rm -fr $(addprefix $O/, $(OBJS))
	rm -f $(exe)


distclean:	clean
	rm -fr $(DEPS)


-include $(DEPS)
