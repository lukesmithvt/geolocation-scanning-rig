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

CFLAGS := -O3 -std=gnu99 -Wall -g
LFLAGS := -lwiringPi

.PHONY: clean reset
one_shot: bin/one_shot
concurrent: bin/concurrent
menu_mode: bin/menu_mode
test: bin/file_writing

bin/one_shot: one_shot.c
> @mkdir -p build bin log raw
> $(CC) $(CFLAGS) -c $< -o build/one_shot.o
> $(CC) -o $@ build/one_shot.o $(LFLAGS)

bin/concurrent: concurrent.c
> @mkdir -p build bin log raw
> $(CC) $(CFLAGS) -c $< -o build/concurrent.o
> $(CC) -o $@ build/concurrent.o $(LFLAGS)

bin/menu_mode: menu_mode.c
> @mkdir -p build bin
> $(CC) $(CFLAGS) -c $< -o build/menu_mode.o
> $(CC) -o $@ build/menu_mode.o $(LFLAGS)

bin/file_writing: file_writing.c
> @mkdir -p build bin
> $(CC) $(CFLAGS) -c $< -o build/file_writing.o
> $(CC) -o $@ build/file_writing.o $(LFLAGS)

clean:
> $(RM) -r bin build

reset:
> $(RM) -r log raw