TARGET := solve_max_labelled_clique

SOURCES := solve_max_labelled_clique.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lmax_labelled_clique -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_labelled_clique.a libgraph.a libthreads.a libsolver.a

