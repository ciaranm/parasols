/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <graph/is_clique.hh>

using namespace parasols;

auto
parasols::is_clique(const Graph & graph, const std::set<int> & members) -> bool
{
    for (auto & a : members)
        for (auto & b : members)
            if (a != b && ! graph.adjacent(a, b))
                return false;

    return true;
}

