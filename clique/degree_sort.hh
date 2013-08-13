/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef CLIQUE_GUARD_CLIQUE_DEGREE_SORT_HH
#define CLIQUE_GUARD_CLIQUE_DEGREE_SORT_HH 1

#include <clique/graph.hh>

#include <vector>

namespace clique
{
    /**
     * Sort the vertices of p in non-decreasing degree order, tie-breaking on
     * vertex number.
     */
    auto degree_sort(const Graph & graph, std::vector<int> & p) -> void;
}

#endif
