TARGET := about_graph

SOURCES := about_graph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lgraph $(boost_ldlibs)
TGT_PREREQS := libgraph.a

