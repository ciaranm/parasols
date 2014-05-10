/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#ifndef PARASOLS_GUARD_GRAPH_MERGE_CLIQUES_HH
#define PARASOLS_GUARD_GRAPH_MERGE_CLIQUES_HH 1

#include <set>
#include <functional>

namespace parasols
{
    /**
     * Given a graph G (via its adjacency relation), and maximal cliques C1 and
     * C2, return the maximum clique in G[C1 union C2].
     */
    auto merge_cliques(const std::function<bool (int, int)> &, const std::set<int> & clique1, const std::set<int> & clique2) -> std::set<int>;
}

#endif
