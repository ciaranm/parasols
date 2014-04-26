TARGET := merge_cliques

SOURCES := merge_cliques.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lgraph $(boost_ldlibs)
TGT_PREREQS := libgraph.a


