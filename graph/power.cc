/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/power.hh>
#include <graph/kneighbours.hh>
#include <limits>

using namespace parasols;

Graph parasols::power(const Graph & graph, int n)
{
    KNeighbours distances(graph, n);
    Graph result((graph));

    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (i != j && distances.vertices[i].distances[j].distance > 0)
                result.add_edge(i, j);

    return result;
}

