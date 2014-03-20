/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/is_club.hh>
#include <graph/kneighbours.hh>
#include <limits>

using namespace parasols;

auto
parasols::is_club(const Graph & graph, int k, const std::vector<int> & members) -> bool
{
    std::vector<int> restrict_to((graph.size()));
    for (auto & m : members)
        restrict_to[m] = 1;

    KNeighbours distances(graph, k, &restrict_to);

    for (auto & i : members)
        for (auto & j : members)
            if (i != j && distances.vertices[i].distances[j].distance <= 0)
                return false;

    return true;
}

