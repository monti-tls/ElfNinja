include conf.mk

SRC_DIR ?= src
INC_DIR ?= inc

ifeq ($(PROGRAM),)
$(error "conf.mk must define the PROGRAM variable")
endif

TMP_DIR = obj
BIN_DIR = bin
SRC_EXT = c

CC = gcc
LD = gcc

CC_FLAGS += -std=c11 -Wall -fPIE -fvisibility=hidden
CC_FLAGS += -I$(INC_DIR)
CC_FLAGS += -O0 -g -ggdb
LD_FLAGS += -pie -rdynamic -L../$(BIN_DIR)

EXE = $(BIN_DIR)/$(PROGRAM)

SOURCES=$(shell find $(SRC_DIR) -name *.$(SRC_EXT))
OBJECTS=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(SOURCES))
DEPMAPS=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.d,$(SOURCES))

# Ignore test files when building shared lib
SOURCES := $(filter-out $(T_SOURCES),$(SOURCES))

all: $(EXE)

clean:
	@rm -rf $(TMP_DIR) $(BIN_DIR)

-include $(DEPMAPS)

$(EXE): $(OBJECTS)
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -o $@
	@echo "(LD) $@"

$(TMP_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(@D)
	@$(CC) $(CC_FLAGS) -MMD -c $< -o $@
	@echo "(CC) $<"
