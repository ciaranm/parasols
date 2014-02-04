/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/complement.hh>

using namespace parasols;

auto parasols::complement(const Graph & graph) -> Graph
{
    Graph result(graph.size(), graph.add_one_for_output());

    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (i != j && ! graph.adjacent(i, j))
                result.add_edge(i, j);

    return result;
}

