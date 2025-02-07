MAKEFLAGS += -rR
.SUFFIXES:

SYSTEM := Unknown

ifeq (($OS),Windows_NT)
	SYSTEM := Windows
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		SYSTEM := Linux
	endif
	ifeq ($(UNAME_S),Darwin)
		SYSTEM := OSX
	endif
endif

ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
	ARCH := x86
endif

O := .O
CC := gcc
CFLAGS := -g -O0 -std=gnu11

exe := vtop

libs := jansson
OBJS := main.o log.o net.o util.o client.o

## Arch specific

OBJS.x86 :=
INC.x86 := 

## OS specific

OBJS.Linux := xev.epoll.o xev.uring.o
CFLAGS.Linux := -DXEV_BACKEND_EPOLL -DXEV_BACKEND_POLL -DXEV_BACKEND_URING

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
	rm -fr $(addprefix $O/, $(OBJS))
	rm -f $(exe)


distclean:	clean
	rm -fr $(DEPS)


-include $(DEPS)
