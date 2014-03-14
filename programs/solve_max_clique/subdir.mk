TARGET := solve_max_clique

SOURCES := solve_max_clique.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lmax_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_clique.a libgraph.a libthreads.a libsolver.a

