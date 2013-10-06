TARGET := solve_max_biclique

SOURCES := solve_max_biclique.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lmax_biclique -lthreads -lgraph
TGT_PREREQS := libmax_biclique.a libgraph.a libthreads.a libsolver.a

