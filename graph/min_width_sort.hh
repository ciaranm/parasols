/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_MIN_WIDTH_SORT_HH
#define PARASOLS_GUARD_GRAPH_MIN_WIDTH_SORT_HH 1

#include <graph/graph.hh>

#include <vector>

namespace parasols
{
    /**
     * Sort the vertices of p in degeneracy / minimum width order.
     */
    auto min_width_sort(const Graph & graph, std::vector<int> & p, bool reverse) -> void;

    /**
     * San Segundo's MWSI. Not clear how to reverse this one.
     */
    auto mwsi_sort(const Graph & graph, std::vector<int> & p) -> void;

    /**
     * Static vertex support in San Segundo's MWSI.
     */
    auto mwssi_sort(const Graph & graph, std::vector<int> & p) -> void;
}

#endif
