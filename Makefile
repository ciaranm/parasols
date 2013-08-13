all : solve_max_clique

CXX = g++-4.7
override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3
override LDFLAGS += `if test -f \`$(CXX) --print-file-name=libboost_thread-mt.so\` ; \
	   then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
	   else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi` \
	   -lrt

FILES = graph/graph \
	graph/bit_graph \
	graph/dimacs \
	threads/atomic_incumbent \
	threads/output_lock \
	max_clique/max_clique_params \
	max_clique/max_clique_result \
	max_clique/print_incumbent \
	max_clique/colourise \
	max_clique/degree_sort \
	max_clique/queue \
	max_clique/naive_max_clique \
	max_clique/mcsa1_max_clique \
	max_clique/tmcsa1_max_clique \
	max_clique/bmcsa1_max_clique \
	max_clique/tbmcsa1_max_clique

CLIQUEOBJECTS = $(foreach c,$(FILES),$(c).o)
OBJECTS = $(CLIQUEOBJECTS) solve_max_clique.o
HEADERS = $(foreach c,$(FILES),$(c).hh)
SOURCES = $(foreach c,$(FILES),$(c).hh)

$(OBJECTS) : %.o : %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

solve_max_clique : $(CLIQUEOBJECTS) solve_max_clique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJECTS) solve_max_clique


