SUBMAKEFILES := \
	graph/graph.mk \
	max_biclique/max_biclique.mk \
	max_clique/max_clique.mk \
	roommates/roommates.mk \
	solver/solver.mk \
	threads/threads.mk \
	create_random_bipartite_graph.mk \
	create_random_graph.mk \
	roommates_frequencies.mk \
	solve_balanced_pairs.mk \
	solve_max_biclique.mk \
	solve_max_clique.mk \
	solve_roommates.mk

BUILD_DIR := $(shell echo intermediate/`hostname`)
TARGET_DIR := $(shell echo build/`hostname`)

boost_ldflags := $(shell if test -f `$(CXX) $$CXXFLAGS $$LDFLAGS --print-file-name=libboost_thread-mt.so` ; \
    then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
    else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi )

CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3
LDFLAGS += $(boost_ldflags) -lrt

