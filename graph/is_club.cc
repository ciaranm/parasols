/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/is_club.hh>
#include <limits>

using namespace parasols;

auto
parasols::is_club(const Graph & graph, int k, const std::set<int> & members) -> bool
{
    std::vector<std::vector<int> > distances((graph.size()));
    for (auto & d : distances)
        d.resize((graph.size()));

    for (int i : members)
        for (int j : members) {
            if (i == j)
                distances[i][j] = 0;
            else if (members.count(i) && members.count(j) && graph.adjacent(i, j))
                distances[i][j] = 1;
            else
                distances[i][j] = std::numeric_limits<int>::max();
        }

    /* shorten */
    for (int k : members)
        for (int i : members)
            for (int j : members)
                if (distances[i][k] != std::numeric_limits<int>::max() &&
                        distances[k][j] != std::numeric_limits<int>::max()) {
                    int d = distances[i][k] + distances[k][j];
                    if (distances[i][j] > d)
                        distances[i][j] = d;
                }

    for (auto & i : members)
        for (auto & j : members)
            if (distances[i][j] > k)
                return false;

    return true;
}

