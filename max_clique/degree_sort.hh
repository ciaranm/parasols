/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_MAX_CLIQUE_DEGREE_SORT_HH
#define PARASOLS_GUARD_MAX_CLIQUE_DEGREE_SORT_HH 1

#include <graph/graph.hh>

#include <vector>

namespace parasols
{
    /**
     * Sort the vertices of p in non-decreasing degree order, tie-breaking on
     * vertex number.
     */
    auto degree_sort(const Graph & graph, std::vector<int> & p) -> void;
}

#endif
