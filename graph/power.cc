/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/power.hh>
#include <limits>

using namespace parasols;

Graph parasols::power(const Graph & graph, int n)
{
    std::vector<std::vector<int> > distances((graph.size()));
    for (auto & d : distances)
        d.resize((graph.size()));

    /* initial distances */
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j) {
            if (i == j)
                distances[i][j] = 0;
            else if (graph.adjacent(i, j))
                distances[i][j] = 1;
            else
                distances[i][j] = std::numeric_limits<int>::max();
        }

    /* shorten */
    for (int k = 0 ; k < graph.size() ; ++k)
        for (int i = 0 ; i < graph.size() ; ++i)
            for (int j = 0 ; j < graph.size() ; ++j)
                if (distances[i][k] != std::numeric_limits<int>::max() &&
                        distances[k][j] != std::numeric_limits<int>::max()) {
                    int d = distances[i][k] + distances[k][j];
                    if (distances[i][j] > d)
                        distances[i][j] = d;
                }

    Graph result;
    result.resize(graph.size());

    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (i != j && distances[i][j] <= n)
                result.add_edge(i, j);

    return result;
}

