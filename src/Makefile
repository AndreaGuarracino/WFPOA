###############################################################################
# Flags & Folders
###############################################################################
FOLDER_BIN=bin
FOLDER_BUILD=build

UNAME=$(shell uname)

CC=gcc
CPP=g++

LD_FLAGS=-lm
CC_FLAGS=-Wall -g
ifeq ($(UNAME), Linux)
  LD_FLAGS+=-lrt 
endif

###############################################################################
# Compile rules
###############################################################################
SUBDIRS=alignment \
        edit \
        system \
        utils
       
all: CC_FLAGS+=-O3
all: MODE=all
all: setup
all: $(SUBDIRS) tools 

debug: setup
debug: MODE=all
debug: $(SUBDIRS) tools

# ASAN: ASAN_OPTIONS=detect_leaks=1:symbolize=1 LSAN_OPTIONS=verbosity=2:log_threads=1
asan: CC_FLAGS+=-fsanitize=address -fno-omit-frame-pointer -fno-common
asan: MODE=all
asan: setup
asan: $(SUBDIRS) tools

setup:
	@mkdir -p $(FOLDER_BIN) $(FOLDER_BUILD)

clean:
	rm -rf $(FOLDER_BIN) $(FOLDER_BUILD)
	
###############################################################################
# Subdir rule
###############################################################################
export
$(SUBDIRS):
	$(MAKE) --directory=$@ all
	
tools:
	$(MAKE) --directory=$@ $(MODE)

.PHONY: $(SUBDIRS) tools
.FORCE:

