###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT=..
FOLDER_BUILD=../build
FOLDER_BIN=../bin

###############################################################################
# Tools
###############################################################################
TOOLS=align_wfe_poa
TOOLS_SRC=$(addsuffix .c, $(TOOLS))

###############################################################################
# Rules
###############################################################################
OBJS=$(FOLDER_BUILD)/*.o

all: LIBS+=$(LD_FLAGS)
all: FLAGS=$(CC_FLAGS)
all: $(TOOLS)

align_wfe_poa: $(FOLDER_BUILD)/*.o align_wfe_poa.c
	$(CC) $(FLAGS) -I$(FOLDER_ROOT) align_wfe_poa.c $(OBJS) -o $(FOLDER_BIN)/align_wfe_poa $(LIBS)
