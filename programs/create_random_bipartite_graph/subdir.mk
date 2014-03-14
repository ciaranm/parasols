TARGET := create_random_bipartite_graph

SOURCES := create_random_bipartite_graph.cc

TGT_LDLIBS := $(boost_ldlibs) -lrt
