/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_POWER_HH
#define PARASOLS_GUARD_GRAPH_POWER_HH 1

#include <graph/graph.hh>

namespace parasols
{
    /**
     * Raise graph to the nth power.
     */
    Graph power(const Graph & graph, int n);
}

#endif
