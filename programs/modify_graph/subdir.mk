TARGET := modify_graph

SOURCES := modify_graph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lgraph $(boost_ldlibs)
TGT_PREREQS := libgraph.a

