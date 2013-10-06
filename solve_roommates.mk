TARGET := solve_roommates

SOURCES := solve_roommates.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lroommates
TGT_PREREQS := libroommates.a

