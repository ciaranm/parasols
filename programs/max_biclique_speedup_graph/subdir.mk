TARGET := max_biclique_speedup_graph

SOURCES := max_biclique_speedup_graph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -Wl,--no-as-needed -lmax_biclique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_biclique.a libgraph.a libthreads.a

