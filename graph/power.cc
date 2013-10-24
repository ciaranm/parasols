/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/power.hh>

using namespace parasols;

Graph parasols::power(const Graph & graph, int n)
{
    /* this is spectacularly dumb. we can do a lot better. */
    Graph result = graph;

    for (int p = 1 ; p < n ; ++p) {
        Graph prev_result = result;

        for (int i = 0 ; i < graph.size() ; ++i) {
            for (int j = 0 ; j < graph.size() ; ++j) {
                if (i == j)
                    continue;

                if (prev_result.adjacent(i, j)) {
                    for (int k = 0 ; k < graph.size() ; ++k) {
                        if (j == k)
                            continue;

                        if (graph.adjacent(i, k))
                            result.add_edge(j, k);
                    }
                }
            }
        }
    }

    return result;
}

