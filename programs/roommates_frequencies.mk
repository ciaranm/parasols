TARGETS := roommates_frequencies

SOURCES := roommates_frequencies.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lroommates
TGT_PREREQS := libroommates.a

