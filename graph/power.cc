/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/power.hh>
#include <limits>

using namespace parasols;

Graph parasols::power(const Graph & graph, int n)
{
    std::vector<std::vector<int> > distances((graph.size()));
    for (auto & d : distances)
        d.resize((graph.size()));

    /* d(m, n) = infinity */
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            distances[i][j] = std::numeric_limits<int>::max();

    /* d(m, m) = 0 */
    for (int i = 0 ; i < graph.size() ; ++i)
        distances[i][i] = 0;

    /* d(m, n) = 1 if m, n in members and adjacent */
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            if (i != j && graph.adjacent(i, j))
                distances[i][j] = 1;

    /* shorten */
    for (int i = 0 ; i < graph.size() ; ++i)
        for (int j = 0 ; j < graph.size() ; ++j)
            for (int k = 0 ; k < graph.size() ; ++k) {
                if (distances[i][k] != std::numeric_limits<int>::max() &&
                        distances[k][j] != std::numeric_limits<int>::max()) {
                    int d = distances[i][k] + distances[k][j];
                    if (distances[i][j] > d)
                        distances[i][j] = d;
                }
            }

    Graph result;
    result.resize(graph.size());

    for (int i = 0 ; i < graph.size() ; ++i) {
        for (int j = 0 ; j < graph.size() ; ++j) {
            if (i == j)
                continue;
            if (distances[i][j] <= n)
                result.add_edge(i, j);
        }
    }

    return result;
}

