/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_DKRTJ_SORT_HH
#define PARASOLS_GUARD_GRAPH_DKRTJ_SORT_HH 1

#include <graph/graph.hh>

#include <vector>

namespace parasols
{
    /**
     * Sort the vertices of p to match the ordering used by DKRTJ.
     *
     * This isn't quite the same as the ordering described in the paper. It's
     * very close to what the implementation does.
     */
    auto dkrtj_sort(const Graph & graph, std::vector<int> & p) -> void;
}

#endif
