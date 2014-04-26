/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_MERGE_CLIQUES_HH
#define PARASOLS_GUARD_GRAPH_MERGE_CLIQUES_HH 1

#include <graph/graph.hh>
#include <set>

namespace parasols
{
    /**
     * Given a graph G and maximal cliques C1 and C2, return the maximum clique
     * in G[C1 union C2].
     */
    auto merge_cliques(const Graph &, const std::set<int> & clique1, const std::set<int> & clique2) -> std::set<int>;
}

#endif
