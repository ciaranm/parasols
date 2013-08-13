all : max_clique

CXX = g++-4.7
override CXXFLAGS += -O3 -march=native -std=c++11 -I./ -W -Wall -g -ggdb3
override LDFLAGS += `if test -f \`$(CXX) --print-file-name=libboost_thread-mt.so\` ; \
	   then echo -lboost_regex-mt -lboost_thread-mt -lboost_system-mt -lboost_program_options-mt ; \
	   else echo -lboost_regex -lboost_thread -lboost_system -lboost_program_options ; fi` \
	   -lrt

FILES = graph/graph \
	graph/bit_graph \
	graph/dimacs \
	clique/max_clique_params \
	clique/colourise \
	clique/degree_sort \
	clique/queue \
	clique/naive_max_clique \
	clique/mcsa1_max_clique \
	clique/tmcsa1_max_clique \
	clique/bmcsa1_max_clique \
	clique/tbmcsa1_max_clique

CLIQUEOBJECTS = $(foreach c,$(FILES),$(c).o)
OBJECTS = $(CLIQUEOBJECTS) max_clique.o
HEADERS = $(foreach c,$(FILES),$(c).hh)
SOURCES = $(foreach c,$(FILES),$(c).hh)

$(OBJECTS) : %.o : %.cc $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

max_clique : $(CLIQUEOBJECTS) max_clique.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean :
	rm -f $(OBJECTS) max_clique


