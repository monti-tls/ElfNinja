### Global configuration

PROJECT = elfninja

### Directories and extensions configuration

SRC_DIR = src
INC_DIR = inc
TMP_DIR = obj
BIN_DIR = bin

SRC_EXT = cpp

### Tools

CC = g++
LD = g++
AR = ar
RM = rm

### Compilation flags

CC_FLAGS  = -std=gnu++11 -Wall
#CC_FLAGS += -g -ggdb -O0
CC_FLAGS += -O2
LD_FLAGS  = -ldl

### Files

PROGRAM     = $(BIN_DIR)/$(PROJECT)
STATIC_LIB  = $(BIN_DIR)/lib$(PROJECT).a
DYNAMIC_LIB = $(BIN_DIR)/lib$(PROJECT).so

LIB_SRC  = $(shell find $(SRC_DIR)/*/ -name *.$(SRC_EXT))
PROG_SRC = $(LIB_SRC) $(wildcard $(SRC_DIR)/*.$(SRC_EXT))

LIB_OBJ  = $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(LIB_SRC))
PROG_OBJ = $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.o,$(PROG_SRC))

### Generated compilation flags

CC_FLAGS += -fPIC -I$(INC_DIR)

### Top-level targets

all: $(PROGRAM) $(STATIC_LIB) $(DYNAMIC_LIB)

.PHONY: clean
clean:
	@$(RM) -rf $(TMP_DIR) $(BIN_DIR)

### Dependencies

DEPS = $(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(TMP_DIR)/%.d,$(PROG_SRC))
-include $(DEPS)

### Final products

$(PROGRAM): $(PROG_OBJ)
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -o $@
	@echo "(LD) $@"

$(STATIC_LIB): $(LIB_OBJ)
	@mkdir -p $(@D)
	@$(AR) rcs $@ $^
	@echo "(AR) $@"

$(DYNAMIC_LIB): $(LIB_OBJ)
	@mkdir -p $(@D)
	@$(LD) $^ $(LD_FLAGS) -shared -o $@
	@echo "(LD) $@"

### Translation rules

$(TMP_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT)
	@mkdir -p $(@D)
	@$(CC) -MMD $(CC_FLAGS) -c $< -o $@
	@echo "(CC) $<"
