include conf.mk

SRC_DIR ?= src
INC_DIR ?= inc

ifeq ($(MODULE),)
$(error "conf.mk must define the MODULE variable")
endif

TMP_DIR = obj
BIN_DIR = bin
SRC_EXT = c

CC = gcc
LD = gcc

CC_FLAGS += -std=c11 -Wall -fPIC
CC_FLAGS += -I$(INC_DIR)
CC_FLAGS += -O0 -g -ggdb
LD_FLAGS += -L../$(BIN_DIR)

LIB_SO = $(BIN_DIR)/libelfninja_$(MODULE).so
LIB_STATIC = $(BIN_DIR)/libelfninja_$(MODULE)-static.a

SOURCES=$(shell find $(SRC_DIR) -name *.$(SRC_EXT))

T_SOURCES=$(shell find $(SRC_DIR) -name *.test.$(SRC_EXT))
T_OBJECTS=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(T_SOURCES))
TESTS=$(patsubst $(SRC_DIR)/%.test.$(SRC_EXT),$(BIN_DIR)/%,$(T_SOURCES))

# Ignore test files when building shared lib
SOURCES := $(filter-out $(T_SOURCES),$(SOURCES))
OBJECTS=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(SOURCES))
DEPMAPS=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.d,$(SOURCES))

# Keep all object files
.SECONDARY: $(T_OBJECTS)

all: $(LIB_SO) $(LIB_STATIC) $(TESTS)

clean:
	@rm -rf $(TMP_DIR) $(BIN_DIR)

-include $(DEPMAPS)

$(LIB_SO): $(OBJECTS)
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -shared -o $@
	@echo "(LD) $@"

$(LIB_STATIC): $(OBJECTS)
	@mkdir -p $(@D)
	@$(AR) rcs $@ $^
	@echo "(AR) $@"

$(BIN_DIR)/%: $(OBJECTS) $(TMP_DIR)/%.test.o
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -o $@
	@echo "(LD) $@"

$(TMP_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(@D)
	@$(CC) $(CC_FLAGS) -MMD -c $< -o $@
	@echo "(CC) $<"
