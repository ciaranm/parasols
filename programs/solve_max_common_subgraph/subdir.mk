TARGET := solve_max_common_subgraph

SOURCES := solve_max_common_subgraph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lmax_common_subgraph -lmax_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_common_subgraph.a libmax_clique.a libgraph.a libthreads.a libsolver.a

