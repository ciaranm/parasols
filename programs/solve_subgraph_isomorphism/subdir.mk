TARGET := solve_subgraph_isomorphism

SOURCES := solve_subgraph_isomorphism.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lsubgraph_isomorphism -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libsubgraph_isomorphism.a libgraph.a libthreads.a libsolver.a


