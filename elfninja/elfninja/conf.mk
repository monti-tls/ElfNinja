PROGRAM = elfninja
CC_FLAGS += -I../core/inc -I../dump/inc -I../input/inc
LD_FLAGS = -lelfninja_core-static -lelfninja_dump-static -lelfninja_input-static -ldl

CC_FLAGS += -DENJ_VERSION=\"0.0\"
CC_FLAGS += -DENJ_BUILD_DATE=\"$(shell date --iso=seconds)\"
