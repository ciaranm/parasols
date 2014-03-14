TARGET := max_clique_graph

SOURCES := max_clique_graph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lmax_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_clique.a libgraph.a libthreads.a

