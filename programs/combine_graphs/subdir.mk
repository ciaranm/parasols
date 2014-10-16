TARGET := combine_graphs

SOURCES := combine_graphs.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lgraph $(boost_ldlibs)
TGT_PREREQS := libgraph.a

