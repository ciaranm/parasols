TARGET := subgraph_isomorphism_association_density_graph

SOURCES := subgraph_isomorphism_association_density_graph.cc

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS := -lsolver -lsubgraph_isomorphism -lthreads -lgraph $(boost_ldlibs)
TGT_PREREQS := libsubgraph_isomorphism.a libgraph.a libthreads.a libsolver.a


