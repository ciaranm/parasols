TARGET := max_common_subgraph_heatmap

SOURCES := max_common_subgraph_heatmap.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -Wl,--no-as-needed -lmax_common_subgraph -lmax_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_common_subgraph.a libmax_clique.a libgraph.a libthreads.a

