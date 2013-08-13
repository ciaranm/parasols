all : solve_max_clique solve_max_biclique

CXX = g++-4.7
override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3
override LDFLAGS += `if test -f \`$(CXX) --print-file-name=libboost_thread-mt.so\` ; \
	   then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
	   else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi` \
	   -lrt

COMMON_FILES = \
	graph/graph \
	graph/bit_graph \
	graph/dimacs \
	graph/degree_sort \
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
	max_biclique/naive_max_biclique

COMMON_OBJECTS = $(foreach c,$(COMMON_FILES),$(c).o)
CLIQUE_OBJECTS = $(foreach c,$(CLIQUE_FILES),$(c).o)
BICLIQUE_OBJECTS = $(foreach c,$(BICLIQUE_FILES),$(c).o)

FILES = $(COMMON_FILES) $(CLIQUE_FILES) $(BICLIQUE_FILES)
OBJECTS = $(COMMON_OBJECTS) $(CLIQUE_OBJECTS) $(BICLIQUE_OBJECTS) solve_max_clique.o solve_max_biclique.o

HEADERS = $(foreach c,$(FILES),$(c).hh)
SOURCES = $(foreach c,$(FILES),$(c).hh)

$(OBJECTS) : %.o : %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

solve_max_clique : $(COMMON_OBJECTS) $(CLIQUE_OBJECTS) solve_max_clique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

solve_max_biclique : $(COMMON_OBJECTS) $(BICLIQUE_OBJECTS) solve_max_biclique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJECTS) solve_max_clique solve_max_biclique

