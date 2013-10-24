TARGET := roommates_frequencies

SOURCES := roommates_frequencies.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lroommates $(boost_ldlibs)
TGT_PREREQS := libroommates.a

