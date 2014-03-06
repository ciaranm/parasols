/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_DEGREE_SORT_HH
#define PARASOLS_GUARD_GRAPH_DEGREE_SORT_HH 1

#include <graph/graph.hh>

#include <vector>

namespace parasols
{
    /**
     * Sort the vertices of p in non-decreasing degree order (or non-increasing
     * if 'reverse' is true), tie-breaking on vertex number.
     */
    auto degree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void;

    /**
     * Sort the vertices of p in non-decreasing degree order (or non-increasing
     * if 'reverse' is true), tie-breaking on exdegree then vertex number.
     */
    auto exdegree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void;

    /**
     * Sort the vertices of p in non-decreasing dynamic degree order (or non-increasing
     * if 'reverse' is true), tie-breaking on non-dynamic exdegree then vertex number.
     */
    auto dynexdegree_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void;

    /**
     * Don't sort the vertices of p.
     */
    auto none_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void;
}

#endif
