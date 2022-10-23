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


menu_mode: menu_mode.c
> $(CC) $(CFLAGS) -c $< -o $@
> $(CC) -o $@ $^ $(LFLAGS) 
