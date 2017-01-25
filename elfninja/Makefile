MODULES = core dump input
PROGRAMS = elfninja
BIN_DIR = bin

DIST_FILES  = $(foreach mod,$(MODULES),$(BIN_DIR)/libelfninja_$(mod).so $(BIN_DIR)/libelfninja_$(mod)-static.a)
DIST_FILES += $(foreach prog,$(PROGRAMS),$(BIN_DIR)/$(prog))

all: $(DIST_FILES)

.PHONY: clean
clean: programs-clean modules-clean
	@rm -rf $(BIN_DIR)

modules-clean: $(foreach mod,$(MODULES),subdir-$(mod)-clean)
programs-clean: $(foreach prog,$(PROGRAMS),subdir-$(prog)-clean)

subdir-%-clean:
	@$(MAKE) --no-print-directory -C $* clean

$(BIN_DIR)/libelfninja_%.so: FORCE
	@mkdir -p $(@D)
	@$(MAKE) --no-print-directory -C $*
	@cp $*/$@ $(BIN_DIR)/

$(BIN_DIR)/libelfninja_%-static.a: $(BIN_DIR)/libelfninja_%.so
	@mkdir -p $(@D)
	@cp $*/$@ $(BIN_DIR)/

$(BIN_DIR)/%: FORCE
	@mkdir -p $(@D)
	@$(MAKE) --no-print-directory -C $*
	@cp $*/$@ $(BIN_DIR)/

FORCE:
	@true
