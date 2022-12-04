SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules

# Some text editors will swap 1 tab for 4 spaces; Make does not like that. 
# This assignment allows tabs to be replaced with '>'.
ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. \
  Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >


SRCS := $(shell find src -name '*.c')
DEPS := $(shell find include -name '*.h')
OBJS := $(patsubst src/%,build/%,$(SRCS:.c=.o))

CFLAGS := -O3 -std=gnu99 -Wall -g
LFLAGS := -lwiringPi


concurrent: bin/concurrent exp0rt

bin/concurrent: $(OBJS)
> @mkdir -p bin
> $(CC) -o $@ $^ $(LFLAGS) 

$(OBJS): build/%.o: src/%.c $(DEPS)
> @mkdir -p build
> $(CC) $(CFLAGS) -c $< -Iinclude -o $@


.PHONY: clean reset exp0rt

clean:
> $(RM) -r bin build

reset:
> $(RM) -r log raw

exp0rt:
> @cp ./bin/concurrent ../../cputemp