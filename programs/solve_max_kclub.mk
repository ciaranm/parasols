TARGET := solve_max_kclub

SOURCES := solve_max_kclub.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lmax_kclub -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libmax_kclub.a libgraph.a libthreads.a libsolver.a

