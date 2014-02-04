SUBMAKEFILES := \
	graph/graph.mk \
	max_biclique/max_biclique.mk \
	max_clique/max_clique.mk \
	max_clique/max_clique_mpi.mk \
	programs/create_random_bipartite_graph.mk \
	programs/create_random_graph.mk \
	programs/max_biclique_speedup_graph.mk \
	programs/max_clique_graph.mk \
	programs/max_clique_speedup_graph.mk \
	programs/roommates_frequencies.mk \
	programs/solve_balanced_pairs.mk \
	programs/solve_max_biclique.mk \
	programs/solve_max_clique.mk \
	programs/solve_roommates.mk \
	programs/solve_vertex_colouring.mk \
	programs/test_max_clique.mk \
	programs/mpi_max_clique.mk \
	roommates/roommates.mk \
	solver/solver.mk \
	threads/threads.mk \
	vertex_colour/vertex_colour.mk

BUILD_DIR := $(shell echo intermediate/`hostname`)
TARGET_DIR := $(shell echo build/`hostname`)

boost_ldlibs := $(shell if test -f `$(CXX) $$CXXFLAGS $$LDFLAGS --print-file-name=libboost_thread-mt.so` ; \
    then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
    else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi )

boost_mpi_ldlibs := -lboost_mpi -lboost_serialization

override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3 -pthread
override LDFLAGS += -pthread

MPICXX = mpic++

