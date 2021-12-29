###############################################################################
# Definitions
###############################################################################
FOLDER_ROOT=../..
FOLDER_BUILD=../../build

###############################################################################
# Modules
###############################################################################
MODULES=edit_wavefront_poa_align \
        edit_wavefront_poa_backtrace \
        edit_wavefront_poa_connect \
        edit_wavefront_poa_display \
        edit_wavefront_poa_extend \
        edit_wavefront_poa
        
SRCS=$(addsuffix .c, $(MODULES))
OBJS=$(addprefix $(FOLDER_BUILD)/, $(SRCS:.c=.o))

###############################################################################
# Rules
###############################################################################
all: $(OBJS)

# General building rule
$(FOLDER_BUILD)/%.o : %.c
	$(CC) $(CC_FLAGS) -I$(FOLDER_ROOT) -c $< -o $@
	
