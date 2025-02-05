MAKEFLAGS += -rR
.SUFFIXES:

ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)
	ARCH := x86
endif

O := .O
CC := gcc
CFLAGS := -g -O0 -std=gnu11

exe := vtop

libs := jansson
OBJS := main.o log.o net.o util.o qemu.o

## Arch specific

OBJS.x86 :=
INC.x86 := 

## Computed Variables

OBJS += $(OBJS.$(ARCH))
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
	rm -fr .O/*
	rm -f $(exe)


distclean:	clean


-include $(DEPS)
