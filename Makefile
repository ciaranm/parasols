all : solve_max_clique solve_max_biclique solve_roommates roommates_frequencies solve_balanced_pairs \
	create_random_graph create_random_bipartite_graph

CXX = g++
override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3
override LDFLAGS += `if test -f \`$(CXX) --print-file-name=libboost_thread-mt.so\` ; \
	   then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
	   else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi` \
	   -lrt

GRAPH_FILES = \
	graph/graph \
	graph/bit_graph \
	graph/dimacs \
	graph/degree_sort \

COMMON_FILES = \
	threads/atomic_incumbent \
	threads/output_lock \
	solver/solver

CLIQUE_FILES = \
	max_clique/max_clique_params \
	max_clique/max_clique_result \
	max_clique/print_incumbent \
	max_clique/colourise \
	max_clique/queue \
	max_clique/naive_max_clique \
	max_clique/mcsa1_max_clique \
	max_clique/tmcsa1_max_clique \
	max_clique/bmcsa1_max_clique \
	max_clique/tbmcsa1_max_clique

BICLIQUE_FILES = \
	max_biclique/max_biclique_params \
	max_biclique/max_biclique_result \
	max_biclique/print_incumbent \
	max_biclique/naive_max_biclique \
	max_biclique/cc_max_biclique \
	max_biclique/degree_max_biclique

ROOMMATES_FILES = \
	roommates/roommates_params \
	roommates/roommates_problem \
	roommates/roommates_result \
	roommates/irving_roommates \
	roommates/roommates_file

GRAPH_OBJECTS = $(foreach c,$(GRAPH_FILES),$(c).o)
COMMON_OBJECTS = $(foreach c,$(COMMON_FILES),$(c).o)
CLIQUE_OBJECTS = $(foreach c,$(CLIQUE_FILES),$(c).o)
BICLIQUE_OBJECTS = $(foreach c,$(BICLIQUE_FILES),$(c).o)
ROOMMATES_OBJECTS = $(foreach c,$(ROOMMATES_FILES),$(c).o)

FILES = $(COMMON_FILES) $(GRAPH_FILES) $(CLIQUE_FILES) $(BICLIQUE_FILES) $(ROOMMATES_FILES)
OBJECTS = $(COMMON_OBJECTS) $(GRAPH_OBJECTS) $(CLIQUE_OBJECTS) $(BICLIQUE_OBJECTS) $(ROOMMATES_OBJECTS) \
	  solve_max_clique.o solve_max_biclique.o solve_roommates.o roommates_frequencies.o solve_balanced_pairs.o

HEADERS = $(foreach c,$(FILES),$(c).hh)
SOURCES = $(foreach c,$(FILES),$(c).hh)

$(OBJECTS) : %.o : %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

solve_max_clique : $(COMMON_OBJECTS) $(GRAPH_OBJECTS) $(CLIQUE_OBJECTS) solve_max_clique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

solve_max_biclique : $(COMMON_OBJECTS) $(GRAPH_OBJECTS) $(BICLIQUE_OBJECTS) solve_max_biclique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

solve_roommates : $(COMMON_OBJECTS) $(GRAPH_OBJECTS) $(ROOMMATES_OBJECTS) solve_roommates.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

roommates_frequencies : $(COMMON_OBJECTS) $(GRAPH_OBJECTS) $(ROOMMATES_OBJECTS) roommates_frequencies.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

solve_balanced_pairs : $(COMMON_OBJECTS) solve_balanced_pairs.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

create_random_graph : $(COMMON_OBJECTS) create_random_graph.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

create_random_bipartite_graph : $(COMMON_OBJECTS) create_random_bipartite_graph.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJECTS) solve_max_clique solve_max_biclique solve_roommates roommates_frequencies solve_balanced_pairs \
	    create_random_graph create_random_bipartite_graph

