/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/is_club.hh>
#include <map>

using namespace parasols;

auto
parasols::is_club(const Graph & graph, int k, const std::set<int> & members) -> bool
{
    /* this could be a lot faster. */
    std::map<std::pair<int, int>, int> distances;

    /* d(m, m) = 0 */
    for (auto & m : members)
        distances[std::make_pair(m, m)] = 0;

    /* d(m, n) = 1 if m, n in members and adjacent */
    for (auto & m : members)
        for (auto & n : members)
            if (m != n && graph.adjacent(m, n))
                distances[std::make_pair(m, n)] = 1;

    /* shorten */
    for (auto & i : members)
        for (auto & j : members)
            for (auto & k : members)
                if (distances.count(std::make_pair(i, k)) && distances.count(std::make_pair(k, j))) {
                    int d = distances[std::make_pair(i, k)] + distances[std::make_pair(k, j)];
                    if (! distances.count(std::make_pair(i, j)))
                        distances[std::make_pair(i, j)] = d;
                    else if (distances[std::make_pair(i, j)] > d)
                        distances[std::make_pair(i, j)] = d;
                }

    for (auto & i : members)
        for (auto & j : members)
            if ((! distances.count(std::make_pair(i, j))) || distances[std::make_pair(i, j)] > k)
                return false;

    return true;
}

