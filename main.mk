SUBMAKEFILES := \
	graph/subdir.mk \
	cco/subdir.mk \
	max_biclique/subdir.mk \
	max_clique/subdir.mk \
	max_common_subgraph/subdir.mk \
	max_kclub/subdir.mk \
	max_labelled_clique/subdir.mk \
	programs/about_graph/subdir.mk \
	programs/create_random_bipartite_graph/subdir.mk \
	programs/create_random_graph/subdir.mk \
	programs/create_graph_product/subdir.mk \
	programs/max_biclique_speedup_graph/subdir.mk \
	programs/max_clique_graph/subdir.mk \
	programs/max_clique_speedup_graph/subdir.mk \
	programs/max_common_subgraph_heatmap/subdir.mk \
	programs/merge_cliques/subdir.mk \
	programs/modify_graph/subdir.mk \
	programs/solve_max_biclique/subdir.mk \
	programs/solve_max_clique/subdir.mk \
	programs/solve_max_common_subgraph/subdir.mk \
	programs/solve_max_kclub/subdir.mk \
	programs/solve_max_labelled_clique/subdir.mk \
	programs/solve_vertex_colouring/subdir.mk \
	programs/test_max_clique/subdir.mk \
	solver/subdir.mk \
	threads/subdir.mk \
	vertex_colour/subdir.mk

ifndef DISABLE_MPI

SUBMAKEFILES += \
	max_clique/subdir_mpi.mk \
	programs/mpi_solve_max_clique/subdir_mpi.mk

endif

BUILD_DIR := $(shell echo intermediate/`hostname`)
TARGET_DIR := $(shell echo build/`hostname`)

boost_ldlibs := -lboost_regex -lboost_thread -lboost_system -lboost_program_options

boost_mpi_ldlibs := -lboost_mpi -lboost_serialization

override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3 -pthread
override LDFLAGS += -pthread

MPICXX = mpic++

