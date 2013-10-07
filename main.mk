SUBMAKEFILES := \
	graph/graph.mk \
	max_biclique/max_biclique.mk \
	max_clique/max_clique.mk \
	programs/create_random_bipartite_graph.mk \
	programs/create_random_graph.mk \
	programs/roommates_frequencies.mk \
	programs/solve_balanced_pairs.mk \
	programs/solve_max_biclique.mk \
	programs/solve_max_clique.mk \
	programs/solve_roommates.mk \
	roommates/roommates.mk \
	solver/solver.mk \
	threads/threads.mk

BUILD_DIR := $(shell echo intermediate/`hostname`)
TARGET_DIR := $(shell echo build/`hostname`)

boost_ldlibs := $(shell if test -f `$(CXX) $$CXXFLAGS $$LDFLAGS --print-file-name=libboost_thread-mt.so` ; \
    then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
    else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi )

override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3

